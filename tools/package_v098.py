"""Package the main build and publish its exact-executable stable manifest."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

subprocess.run([sys.executable,'tools/package_a35.py','43','Q-Protocol_v0.9.8'],check=True)
dist=Path('dist')
check=json.loads((dist/'A43-package-checks.json').read_text(encoding='utf-8'))
check['status']='built/checked; runtime base beta.4 accepted by maintainer'
(dist/'v0.9.8-package-checks.json').write_text(json.dumps(check,indent=2)+'\n',encoding='utf-8')
manifest={'schema':1,'build':43,'tag':'v0.9.8',
    'exe_sha256':'f92c0e8600aa3b692a437f2c0795068f509db1a73a4794b619e6d60bd5d35c43',
    'package':check['package'],
    'url':'https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/download/v0.9.8/'+check['package'],
    'size':check['size'],'sha256':check['sha256'],'files':check['files']}
assert hashlib.sha256((dist/check['package']).read_bytes()).hexdigest()==manifest['sha256']
(dist/'QProtocol-update.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print(json.dumps(manifest,indent=2))
