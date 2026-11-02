"""v0.9.7 changes only default beta and build identity relative to accepted A29."""
from pathlib import Path
import subprocess

base='e04b7de7d7b529e0353ce241befe62a5be2e5504'
for path in ['src/Updates.cpp','src/Updates.h','src/QProtocol.cpp']:
    old=subprocess.check_output(['git','show',f'{base}:{path}'],encoding='utf-8')
    new=Path(path).read_text(encoding='utf-8')
    if path.endswith('Updates.cpp'):
        expected=old.replace('betaChannel{true}','betaChannel{false}').replace('L"Beta",1,path.c_str()', 'L"Beta",0,path.c_str()')
    elif path.endswith('Updates.h'):
        expected=old.replace('Build = 29','Build = 30').replace('"A29 TEST"','"v0.9.7"').replace('checkedBeta = true','checkedBeta = false')
    else:
        expected=old.replace('Q Protocol v0.9.6 / A29 Updater and Menu TEST','Q Protocol v0.9.7 / Updater and Readable Menu').replace('Scope: A28 native core preserved; background updater and menu readability TEST.','Scope: accepted A28/A29 core and menu; stable updates by default.')
    assert new==expected,path
for path in ['src/OverlayA19.cpp','updater/UpdateEngine.cs','config/QProtocol.ini']:
    assert Path(path).read_text(encoding='utf-8')==subprocess.check_output(['git','show',f'{base}:{path}'],encoding='utf-8'),path
print('Release scope PASS: beta default OFF, startup check ON, existing explicit preferences retained; accepted A29 behavior unchanged.')
