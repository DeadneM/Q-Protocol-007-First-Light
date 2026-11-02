using System;
using System.IO;
using System.Web.Script.Serialization;
using QProtocol.Update;
internal static class PublishedProbe {
    public static int Main(string[] args) {
        try {
            var package=Engine.Discover(args[0],29,false);
            if(package==null || package.Build!=30) throw new Exception("v0.9.7 compatible stable not discovered");
            var bytes=Engine.Get(package.Url,32*1024*1024);
            Engine.Extract(bytes,args[1],package);
            if(Engine.Discover(args[0],30,false)!=null) throw new Exception("Current v0.9.7 offered an older or identical release");
            if(Engine.Discover(args[0],29,true)==null) throw new Exception("A29 beta channel missed the stable upgrade");
            Console.WriteLine(new JavaScriptSerializer().Serialize(new {tag=package.Tag,build=package.Build,sha256=package.Sha,size=package.Size,
                compatible_stable_found=true,published_package_verified=true,current_build_not_reoffered=true,A29_beta_channel_sees_stable_upgrade=true,
                game_started=false,game_files_modified=false}));
            return 0;
        } catch(Exception e) {Console.Error.WriteLine(e);return 1;}
    }
}
