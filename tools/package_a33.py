"""Binary-only update package and repository-published compatibility manifest."""
import hashlib
import json
from pathlib import Path
import zipfile

dist=Path('dist')
names=['dxgi.dll','QProtocol.asi']
package='Q-Protocol_A33_TacSimIdentity_TEST.zip'
tag='a33-tacsim-identity-test'
out=dist/package
files={n:{'size':(dist/n).stat().st_size,'sha256':hashlib.sha256((dist/n).read_bytes()).hexdigest()} for n in names}
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for n in names:z.write(dist/n,n)
with zipfile.ZipFile(out) as z:
    assert z.namelist()==names and z.testzip() is None
manifest={'schema':1,'build':33,'tag':tag,'exe_sha256':'6e63df4d2891ea085a71ef9a6f4d24b9db52144879945e8cc1a2e8106fabd3f1',
          'package':package,'url':f'https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/download/{tag}/{package}',
          'size':out.stat().st_size,'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'files':files}
(dist/'QProtocol-update.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print(json.dumps(manifest,indent=2))
