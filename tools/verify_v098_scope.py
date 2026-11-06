"""Promote the maintainer-accepted beta.4 with only the stable build identity."""
from pathlib import Path
import configparser
import subprocess

BASE = '2fd0668e40308e124aaf829b138412ec57c8fb08'
paths = subprocess.check_output(['git','ls-tree','-r','--name-only',BASE,'--','src','config','updater'],encoding='utf-8').splitlines()
for name in paths:
    previous = subprocess.check_output(['git','show',f'{BASE}:{name}'])
    current = Path(name).read_bytes()
    # Git stores LF while Windows can check out CRLF.
    previous = previous.replace(b'\r\n',b'\n')
    current = current.replace(b'\r\n',b'\n')
    if name == 'src/Updates.h':
        previous = previous.replace(b'Build = 42',b'Build = 43').replace(b'"v0.9.8-beta.4"',b'"v0.9.8"')
    assert current == previous, name
actual = set(subprocess.check_output(['git','ls-files','--','src','config','updater'],encoding='utf-8').splitlines())
assert actual == set(paths), 'Unexpected added/removed runtime files'
cfg = configparser.ConfigParser(interpolation=None)
cfg.read('config/QProtocol.ini',encoding='utf-8')
assert cfg.getboolean('Updater','CheckAtStart',fallback=True)
assert not cfg.getboolean('Updater','Beta',fallback=False)
update = Path('src/Updates.cpp').read_text(encoding='utf-8')
assert 'betaChannel{false}' in update and 'L"Beta",0,path.c_str()' in update
assert 'L"CheckAtStart",1,path.c_str()' in update
print('v0.9.8 preservation PASS: complete accepted beta.4 runtime unchanged except stable identity; beta OFF/startup check ON; personal preferences retained.')
