using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using QProtocol.Update;

internal static class UpdaterTests {
    static int checks;
    static void Assert(bool ok,string name) { if(!ok) throw new Exception(name); checks++; }
    static void Reject(Action action,string name) { bool rejected=false; try { action(); } catch { rejected=true; } Assert(rejected,name); }
    static byte[] Pe(byte seed) {
        var b=Enumerable.Repeat(seed,1024).ToArray(); b[0]=77;b[1]=90;
        Array.Copy(BitConverter.GetBytes(64),0,b,60,4);
        Array.Copy(BitConverter.GetBytes(0x4550),0,b,64,4);
        Array.Copy(BitConverter.GetBytes((ushort)0x8664),0,b,68,2);
        Array.Copy(BitConverter.GetBytes((ushort)0x2000),0,b,86,2);
        Array.Copy(BitConverter.GetBytes((ushort)0x20b),0,b,88,2);return b;
    }
    static byte[] Zip(Dictionary<string,byte[]> files) {
        using(var output=new MemoryStream()) {
            using(var zip=new ZipArchive(output,ZipArchiveMode.Create,true))
                foreach(var pair in files) using(var stream=zip.CreateEntry(pair.Key).Open()) stream.Write(pair.Value,0,pair.Value.Length);
            return output.ToArray();
        }
    }
    static Dictionary<string,object> Manifest(byte[] zip,Dictionary<string,byte[]> files,string exe) {
        var meta=new Dictionary<string,object>();
        foreach(var f in files) meta[f.Key]=new Dictionary<string,object>{{"size",f.Value.Length},{"sha256",Engine.Digest(f.Value)}};
        return new Dictionary<string,object>{{"schema",1},{"build",30},{"tag","a30-test"},{"exe_sha256",exe},
            {"package","Q-Protocol_A30_TEST.zip"},{"url","https://github.com/"+Engine.Repo+"/releases/download/a30-test/Q-Protocol_A30_TEST.zip"},
            {"size",zip.Length},{"sha256",Engine.Digest(zip)},{"files",meta}};
    }
    public static int Main() {
        string root=Path.Combine(Path.GetTempPath(),"QP-UpdaterTests-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
        try {
            string folder=Path.Combine(root,"game"),stage=Path.Combine(root,"stage");Directory.CreateDirectory(folder);
            File.WriteAllText(Path.Combine(folder,"007FirstLight.exe"),"fixture only - never executed");
            string exe=Engine.Hash(Path.Combine(folder,"007FirstLight.exe"));
            var files=new Dictionary<string,byte[]>{{"dxgi.dll",Pe(19)},{"QProtocol.asi",Pe(37)}};
            // Stored (uncompressed) archive so the valid fixture also exercises the minimum package size.
            byte[] zip;
            using(var memory=new MemoryStream()) {
                using(var z=new ZipArchive(memory,ZipArchiveMode.Create,true))
                    foreach(var f in files) using(var st=z.CreateEntry(f.Key,CompressionLevel.NoCompression).Open()) st.Write(f.Value,0,f.Value.Length);
                zip=memory.ToArray();
            }
            var m=Manifest(zip,files,exe);
            Package package=Engine.Parse(m,"a30-test",exe,29,true,true);
            Assert(package.Build==30,"newer compatible beta accepted");
            Reject(()=>Engine.Parse(m,"a30-test",exe,29,false,true),"stable rejects beta");
            Reject(()=>Engine.Parse(m,"a30-test",exe,30,true,true),"same build rejected");
            Reject(()=>Engine.Parse(m,"a30-test",exe,31,true,true),"downgrade rejected");
            Reject(()=>Engine.Parse(m,"a30-test",new string('0',64),29,true,true),"wrong game rejected");
            var bad=new Dictionary<string,object>(m);bad["url"]="https://example.com/attack.zip";
            Reject(()=>Engine.Parse(bad,"a30-test",exe,29,true,true),"foreign package URL rejected");
            bad=new Dictionary<string,object>(m);bad["files"]=new Dictionary<string,object>{{"QProtocol.ini",m["files"]}};
            Reject(()=>Engine.Parse(bad,"a30-test",exe,29,true,true),"INI payload rejected");
            Engine.Extract(zip,stage,package);
            Assert(Engine.Hash(Path.Combine(stage,"QProtocol.asi"))==Engine.Digest(files["QProtocol.asi"]),"valid two-binary payload extracted");
            byte[] corrupt=(byte[])zip.Clone();corrupt[100]^=1;
            Reject(()=>Engine.Extract(corrupt,Path.Combine(root,"bad"),package),"corrupt ZIP rejected");
            byte[] x86=Pe(0);Array.Copy(BitConverter.GetBytes((ushort)0x14c),0,x86,68,2);
            Reject(()=>Engine.VerifyPe(x86),"x86 payload rejected");
            var nested=new Dictionary<string,byte[]>{{"../dxgi.dll",Pe(19)},{"QProtocol.asi",Pe(37)}};
            byte[] nestedZip=Zip(nested);var nestedPackage=new Package{Size=nestedZip.Length,Sha=Engine.Digest(nestedZip),Files=package.Files};
            Reject(()=>Engine.Extract(nestedZip,Path.Combine(root,"nested"),nestedPackage),"path traversal rejected");
            byte[] ini={0xff,0xfe,65,0,13,0,10,0};File.WriteAllBytes(Path.Combine(folder,"QProtocol.ini"),ini);
            File.WriteAllText(Path.Combine(folder,"README.txt"),"personal readme");
            foreach(string n in Engine.Names) File.WriteAllText(Path.Combine(folder,n),"old "+n);
            var before=Engine.Names.ToDictionary(n=>n,n=>Engine.Hash(Path.Combine(folder,n)));
            using(var held=new FileStream(Path.Combine(folder,"QProtocol.asi"),FileMode.Open,FileAccess.Read,FileShare.Read))
                Reject(()=>Engine.Apply(folder,stage,package,before),"locked ASI prevents update and rolls back first file");
            Assert(Engine.Names.All(n=>Engine.Hash(Path.Combine(folder,n))==before[n]),"rollback restores previous binaries");
            Assert(!Directory.GetFiles(folder,"*.qp-new").Any(),"failed transaction cleans its temporary files");
            Engine.Apply(folder,stage,package,before);
            Assert(Engine.Names.All(n=>Engine.Hash(Path.Combine(folder,n))==Engine.Digest(files[n])),"both new binaries installed");
            Assert(File.ReadAllBytes(Path.Combine(folder,"QProtocol.ini")).SequenceEqual(ini),"personal INI preserved byte for byte");
            Assert(File.ReadAllText(Path.Combine(folder,"README.txt"))=="personal readme","README untouched");
            Assert(Directory.GetDirectories(Path.Combine(folder,"QProtocol-Updates")).Length>=2,"verified backups retained");
            Reject(()=>Engine.Apply(folder,stage,package,before),"changed installed module rejected");
            Console.WriteLine("Updater tests PASS: "+checks+" checks; fixtures only, no game/network/real installation.");return 0;
        } catch(Exception e) {Console.Error.WriteLine(e);return 1;}
        finally {Directory.Delete(root,true);}
    }
}
