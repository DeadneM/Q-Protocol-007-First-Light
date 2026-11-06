"""Package and verify the two Windows x64 binaries."""
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path

build = int(sys.argv[1]) if len(sys.argv) > 1 else 35
package = sys.argv[2] if len(sys.argv) > 2 else 'Q-Protocol_A35_SessionRepair_TEST'
dist = Path('dist')
names = ['dxgi.dll', 'QProtocol.asi']
records = {}
for name in names:
    data = (dist / name).read_bytes()
    assert data[:2] == b'MZ', name
    offset = struct.unpack_from('<I', data, 0x3c)[0]
    assert data[offset:offset+4] == b'PE\0\0', name
    assert struct.unpack_from('<H', data, offset+4)[0] == 0x8664, name
    records[name] = {'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
out = dist / f'{package}.zip'
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
    for name in names:
        archive.write(dist / name, name)
with zipfile.ZipFile(out) as archive:
    assert archive.namelist() == names and archive.testzip() is None
    for name in names:
        assert hashlib.sha256(archive.read(name)).hexdigest() == records[name]['sha256']
report = {'build': build, 'status': 'experimental; not game-validated', 'files': records,
          'package': out.name, 'size': out.stat().st_size,
          'sha256': hashlib.sha256(out.read_bytes()).hexdigest()}
(dist / f'A{build}-package-checks.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
print(json.dumps(report, indent=2))
