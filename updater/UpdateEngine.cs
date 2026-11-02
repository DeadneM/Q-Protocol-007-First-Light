using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Web.Script.Serialization;

namespace QProtocol.Update {
    internal sealed class Package {
        public int Build; public string Tag, Url, Sha, Exe;
        public long Size; public Dictionary<string,object> Files;
    }
    internal static class Engine {
        internal const string Repo="DeadneM/Q-Protocol-007-First-Light";
        internal static readonly string[] Names={"dxgi.dll","QProtocol.asi"};
        const int Limit=32*1024*1024;
        static readonly JavaScriptSerializer Json=new JavaScriptSerializer { MaxJsonLength=4*1024*1024 };
        internal static Dictionary<string,object> Obj(object x) { return (Dictionary<string,object>)x; }
        internal static string Str(Dictionary<string,object> x,string k) { return Convert.ToString(x[k],System.Globalization.CultureInfo.InvariantCulture); }
        internal static string Digest(byte[] x) { using(var h=SHA256.Create()) return BitConverter.ToString(h.ComputeHash(x)).Replace("-","").ToLowerInvariant(); }
        internal static string Hash(string p) { using(var s=File.OpenRead(p)) using(var h=SHA256.Create()) return BitConverter.ToString(h.ComputeHash(s)).Replace("-","").ToLowerInvariant(); }
        static bool Hex(string s) { return s!=null && Regex.IsMatch(s,"^[0-9a-f]{64}$"); }
        static void Require(bool value,string why) { if(!value) throw new InvalidDataException(why); }
        internal static byte[] Get(string url,int maximum) {
            ServicePointManager.SecurityProtocol=SecurityProtocolType.Tls12;
            Uri uri=new Uri(url);
            for(int redirects=0;redirects<6;redirects++) {
                Require(uri.Scheme=="https" && new[]{"api.github.com","github.com","release-assets.githubusercontent.com","objects.githubusercontent.com"}.Contains(uri.Host),"Untrusted HTTPS destination");
                var req=(HttpWebRequest)WebRequest.Create(uri);
                req.UserAgent="QProtocol-Updater/1"; req.Accept="application/vnd.github+json";
                req.Timeout=15000; req.ReadWriteTimeout=15000; req.AllowAutoRedirect=false;
                using(var response=(HttpWebResponse)req.GetResponse()) {
                    int code=(int)response.StatusCode;
                    if(code==301||code==302||code==303||code==307||code==308) { uri=new Uri(uri,response.Headers["Location"]); continue; }
                    Require(code==200 && response.ContentLength<=maximum,"Invalid or oversized response");
                    using(var input=response.GetResponseStream()) using(var output=new MemoryStream()) {
                        byte[] b=new byte[8192]; int count;
                        while((count=input.Read(b,0,b.Length))>0) { Require(output.Length+count<=maximum,"Response exceeds limit"); output.Write(b,0,count); }
                        return output.ToArray();
                    }
                }
            }
            throw new IOException("Too many HTTPS redirects");
        }
        internal static Package Parse(Dictionary<string,object> m,string tag,string exe,int current,bool beta,bool prerelease) {
            Require(Convert.ToInt32(m["schema"])==1,"Unknown update manifest schema");
            Require(Regex.IsMatch(tag,"^[A-Za-z0-9._-]+$") && Str(m,"tag")==tag,"Invalid release tag");
            int build=Convert.ToInt32(m["build"]);
            Require(build>current,"Not newer than this build");
            Require(beta||!prerelease,"Beta release on stable channel");
            Require(Str(m,"exe_sha256")==exe && Hex(exe),"Game executable is incompatible");
            string name=Str(m,"package"),url=Str(m,"url"),sha=Str(m,"sha256");
            Require(Regex.IsMatch(name,@"^Q-Protocol_[A-Za-z0-9._-]+\.zip$") &&
                    url=="https://github.com/"+Repo+"/releases/download/"+tag+"/"+name,"Invalid package URL");
            long size=Convert.ToInt64(m["size"]);
            Require(size>1000 && size<=Limit && Hex(sha),"Invalid package size or SHA-256");
            var files=Obj(m["files"]);
            Require(files.Count==2 && Names.All(files.ContainsKey),"Only mod binaries are allowed");
            foreach(string file in Names) {
                var info=Obj(files[file]); long length=Convert.ToInt64(info["size"]);
                Require(length>=1024 && length<=Limit && Hex(Str(info,"sha256")),"Invalid binary metadata");
            }
            return new Package { Build=build,Tag=tag,Url=url,Sha=sha,Size=size,Files=files,Exe=exe };
        }
        internal static Package Discover(string folder,int current,bool beta) {
            string exe=Hash(Path.Combine(folder,"007FirstLight.exe"));
            var list=Json.DeserializeObject(Encoding.UTF8.GetString(Get("https://api.github.com/repos/"+Repo+"/releases?per_page=100",4*1024*1024))) as object[];
            Require(list!=null,"Invalid release listing");
            Package latest=null;
            foreach(object item in list) {
                var release=Obj(item); bool pre=Convert.ToBoolean(release["prerelease"]);
                if(Convert.ToBoolean(release["draft"]) || (!beta&&pre)) continue;
                string tag=Str(release,"tag_name");
                if(!Regex.IsMatch(tag,"^[A-Za-z0-9._-]+$")) continue;
                foreach(object a in (object[])release["assets"]) {
                    var asset=Obj(a); if(Str(asset,"name")!="QProtocol-update.json") continue;
                    string url=Str(asset,"browser_download_url");
                    if(url!="https://github.com/"+Repo+"/releases/download/"+tag+"/QProtocol-update.json") continue;
                    // Incompatible or older manifests are normal when switching channels.
                    var m=Obj(Json.DeserializeObject(Encoding.UTF8.GetString(Get(url,64*1024))));
                    if(Convert.ToInt32(m["build"])<=current || Str(m,"exe_sha256")!=exe) continue;
                    Package p=Parse(m,tag,exe,current,beta,pre);
                    if(latest==null || p.Build>latest.Build) latest=p;
                }
            }
            return latest;
        }
        internal static void Status(string state,string phase,string message,Package p=null) {
            Directory.CreateDirectory(state);
            string text="phase="+phase+"\nmessage="+message.Replace('\r',' ').Replace('\n',' ')+"\n";
            if(p!=null) text+="version="+p.Tag+"\nbuild="+p.Build+"\n";
            string file=Path.Combine(state,"status.txt"),temp=file+".tmp";
            File.WriteAllText(temp,text,new UTF8Encoding(false));
            if(File.Exists(file)) File.Replace(temp,file,null); else File.Move(temp,file);
        }
        internal static void VerifyPe(byte[] b) {
            Require(b.Length>=1024 && b[0]=='M' && b[1]=='Z',"Not a PE binary");
            int pe=BitConverter.ToInt32(b,60);
            Require(pe>=64 && pe<=b.Length-26 && BitConverter.ToUInt32(b,pe)==0x4550 &&
                    BitConverter.ToUInt16(b,pe+4)==0x8664 &&
                    (BitConverter.ToUInt16(b,pe+22)&0x2000)!=0 &&
                    BitConverter.ToUInt16(b,pe+24)==0x20b,"Expected AMD64 PE32+ DLL");
        }
        internal static void Extract(byte[] bytes,string stage,Package p) {
            Require(bytes.LongLength==p.Size && Digest(bytes)==p.Sha,"ZIP SHA-256 or size mismatch");
            Directory.CreateDirectory(stage);
            using(var memory=new MemoryStream(bytes)) using(var zip=new ZipArchive(memory,ZipArchiveMode.Read)) {
                Require(zip.Entries.Count==2,"ZIP must contain exactly two binaries");
                var seen=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                foreach(var e in zip.Entries) {
                    Require(Names.Contains(e.FullName) && seen.Add(e.FullName),"Unexpected, nested or duplicate ZIP entry");
                    var expected=Obj(p.Files[e.FullName]);
                    Require(e.Length==Convert.ToInt64(expected["size"]) && e.Length<=Limit,"Unexpected binary size");
                    using(var input=e.Open()) using(var output=new MemoryStream()) {
                        byte[] buffer=new byte[8192]; int n;
                        while((n=input.Read(buffer,0,buffer.Length))>0) { Require(output.Length+n<=e.Length,"Invalid expanded size"); output.Write(buffer,0,n); }
                        byte[] data=output.ToArray();
                        Require(data.LongLength==e.Length && Digest(data)==Str(expected,"sha256"),"Binary SHA-256 mismatch");
                        VerifyPe(data); File.WriteAllBytes(Path.Combine(stage,e.FullName),data);
                    }
                }
            }
        }
        static string OptionalHash(string p) { return File.Exists(p)?Hash(p):"ABSENT"; }
        static void Regular(string p) { Require((!File.Exists(p)&&!Directory.Exists(p)) || (File.GetAttributes(p)&FileAttributes.ReparsePoint)==0,"Reparse-point path refused"); }
        internal static void Apply(string folder,string stage,Package p,Dictionary<string,string> before) {
            Require(Hash(Path.Combine(folder,"007FirstLight.exe"))==p.Exe,"Game changed after staging");
            string ini=Path.Combine(folder,"QProtocol.ini"),iniHash=OptionalHash(ini);
            foreach(string name in Names) {
                string target=Path.Combine(folder,name); Regular(target);
                Require(OptionalHash(target)==before[name],"Installed binary changed after staging");
                Require(Hash(Path.Combine(stage,name))==Str(Obj(p.Files[name]),"sha256"),"Staged binary changed");
            }
            string root=Path.Combine(folder,"QProtocol-Updates");
            Directory.CreateDirectory(root); Regular(root);
            string backup=Path.Combine(root,"before-"+DateTime.UtcNow.ToString("yyyyMMddTHHmmssfff")+"-"+Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(backup);
            foreach(string name in Names) {
                string target=Path.Combine(folder,name);
                if(File.Exists(target)) {
                    File.Copy(target,Path.Combine(backup,name));
                    Require(Hash(Path.Combine(backup,name))==before[name],"Backup integrity failed");
                }
            }
            File.WriteAllText(Path.Combine(backup,"backup.json"),Json.Serialize(before),Encoding.UTF8);
            var changed=new List<string>();
            var temporaryFiles=new List<string>();
            try {
                foreach(string name in Names) {
                    string target=Path.Combine(folder,name),temp=Path.Combine(folder,name+".qp-new");
                    Regular(temp); Require(!File.Exists(temp),"Another update is pending");
                    File.Copy(Path.Combine(stage,name),temp);
                    temporaryFiles.Add(temp);
                    // The already verified backup is kept separately. Never touch INI/README.
                    if(File.Exists(target)) File.Replace(temp,target,null); else File.Move(temp,target);
                    changed.Add(name);
                    Require(Hash(target)==Str(Obj(p.Files[name]),"sha256"),"Installed binary integrity failed");
                }
                Require(OptionalHash(ini)==iniHash,"INI changed during update");
            } catch {
                foreach(string name in changed) {
                    string target=Path.Combine(folder,name),saved=Path.Combine(backup,name);
                    if(File.Exists(saved)) File.Copy(saved,target,true); else if(File.Exists(target)) File.Delete(target);
                }
                throw;
            } finally {
                foreach(string temp in temporaryFiles) if(File.Exists(temp)) File.Delete(temp);
            }
        }
        internal static bool Running(string folder) {
            foreach(var p in Process.GetProcessesByName("007FirstLight")) using(p) {
                try { if(!p.HasExited && string.Equals(Path.GetFullPath(p.MainModule.FileName),Path.Combine(folder,"007FirstLight.exe"),StringComparison.OrdinalIgnoreCase)) return true; }
                catch { return true; }
            }
            return false;
        }
        internal static void Run(string mode,string folder,string state,int pid,int current,bool beta,int selected) {
            folder=Path.GetFullPath(folder); state=Path.GetFullPath(state);
            Require(File.Exists(Path.Combine(folder,"007FirstLight.exe")),"Game EXE not found");
            using(var parent=Process.GetProcessById(pid)) {
                IntPtr handle=parent.Handle; // hold the original process handle, not a later reused PID
                Require(string.Equals(Path.GetFullPath(parent.MainModule.FileName),Path.Combine(folder,"007FirstLight.exe"),StringComparison.OrdinalIgnoreCase),"Wrong parent process");
                Status(state,"checking","Checking GitHub for a compatible "+(beta?"beta":"stable")+" build...");
                Package package=Discover(folder,current,beta);
                if(package==null) { Status(state,"current","No newer compatible "+(beta?"beta":"stable")+" build is published."); return; }
                if(mode=="--check") { Status(state,"available","A newer compatible build is available.",package); return; }
                Require(mode=="--stage","Unknown updater mode");
                Require(package.Build==selected,"Release changed since the check. Check again before confirming.");
                string probe=Path.Combine(folder,".qp-write-probe-"+Guid.NewGuid().ToString("N"));
                using(var f=new FileStream(probe,FileMode.CreateNew,FileAccess.Write,FileShare.None)) f.WriteByte(0);
                File.Delete(probe); // report write-permission failures before the player exits
                var before=Names.ToDictionary(n=>n,n=>OptionalHash(Path.Combine(folder,n)));
                Status(state,"downloading","Downloading and verifying the update...",package);
                string stage=Path.Combine(state,"payload-"+Guid.NewGuid().ToString("N"));
                Extract(Get(package.Url,Limit),stage,package);
                Status(state,"staged","Update ready. Close the game normally to install. Personal INI is kept.",package);
                while(!parent.WaitForExit(500)) {
                    if(File.Exists(Path.Combine(state,"cancel.request"))) { Status(state,"cancelled","Scheduled update cancelled."); return; }
                }
                if(File.Exists(Path.Combine(state,"cancel.request"))) { Status(state,"cancelled","Scheduled update cancelled."); return; }
                Require(!Running(folder),"Another game session is running; update cancelled");
                Apply(folder,stage,package,before);
                Status(state,"installed","Update installed. Personal INI preserved; previous binaries backed up.",package);
            }
        }
    }
    internal static class Program {
        [STAThread] static int Main(string[] args) {
            if(args.Length!=7) return 2;
            try { Engine.Run(args[0],args[1],args[2],int.Parse(args[3]),int.Parse(args[4]),args[5]=="beta",int.Parse(args[6])); return 0; }
            catch(Exception e) { try { Engine.Status(args[2],"error",e.Message); } catch {} return 1; }
        }
    }
}
