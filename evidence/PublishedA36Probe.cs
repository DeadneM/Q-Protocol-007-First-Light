using System;
using System.Web.Script.Serialization;
using QProtocol.Update;
internal static class PublishedProbe {
    public static int Main(string[] args) {
        try {
            var package=Engine.Discover(args[0],34,true);
            if(package==null || package.Build!=36) throw new Exception("A36 compatible beta not discovered");
            var bytes=Engine.Get(package.Url,32*1024*1024);
            Engine.Extract(bytes,args[1],package);
            if(Engine.Discover(args[0],36,true)!=null) throw new Exception("Current A36 offered an older or identical release");
            if(Engine.Discover(args[0],30,false)!=null) throw new Exception("Stable discovery offered a TEST release");
            Console.WriteLine(new JavaScriptSerializer().Serialize(new {tag=package.Tag,build=package.Build,sha256=package.Sha,size=package.Size,
                compatible_beta_found=true,published_package_verified=true,current_build_not_reoffered=true,stable_excludes_beta=true,
                game_started=false,game_files_modified=false}));
            return 0;
        } catch(Exception e) {Console.Error.WriteLine(e);return 1;}
    }
}
