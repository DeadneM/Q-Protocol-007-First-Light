"""Read-only verification of the A38 map against the exact supported PE file."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
from audit_game_executable import audit

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('executable', type=Path)
args = parser.parse_args()
report = json.loads(Path('evidence/nov2026/a38-native-map.json').read_text(encoding='utf-8'))
inventory = audit(args.executable, Path('src/QProtocol.cpp'))
assert inventory['sha256'] == report['exeSha256'], 'Unexpected executable hash'
assert int(inventory['timestamp'], 16) == int(report['timestamp'], 16)
assert int(inventory['size_of_image'], 16) == int(report['sizeOfImage'], 16)
data = args.executable.read_bytes()

def read(rva, count):
    section = next(s for s in inventory['sections']
                   if s['rva'] <= rva and rva + count <= s['rva'] + s['raw_size'])
    offset = section['offset'] + rva - section['rva']
    return data[offset:offset + count]

core = Path('src/QProtocol.cpp').read_text(encoding='utf-8')
player = Path('src/NativePlayerGuards.inl').read_text(encoding='utf-8')
delivery_path = Path('src/NativeDeliveryGuards.inl')
delivery = delivery_path.read_text(encoding='utf-8') if delivery_path.exists() else ''
checks = 0
for source, pattern in [
    (core, r'const BYTE expected\d+\[\] = \{([^}]+)\};\n    if \(!code\("[^"]+", (0x[0-9A-F]+),'),
    (player + delivery, r'const BYTE expected\d+\[\] = \{([^}]+)\};\n[^\n]+\n    if \(!SafeRead\(g_exeBase \+ (0x[0-9A-F]+),')]:
    for raw, rva in re.findall(pattern, source):
        expected = bytes(int(x, 16) for x in re.findall(r'0x([0-9A-F]{2})', raw))
        assert read(int(rva, 16), len(expected)) == expected, rva
        checks += 1
expected_checks = 27 if delivery else 18
assert checks == expected_checks, f'Missing native guards ({checks}/{expected_checks})'
base = int(inventory['image_base'], 16)
links = re.search(r'links\[\]\[2\] = \{([\s\S]+?)\n    \};', core)[1]
for at, target in re.findall(r'\{(0x[0-9A-F]+), (0x[0-9A-F]+)\}', links):
    assert struct.unpack('<Q', read(int(at, 16), 8))[0] == base + int(target, 16), at
method = re.search(r'g_exeBase \+ (0x[0-9A-F]+), method\).*?method != g_exeBase \+ (0x[0-9A-F]+)', player)
assert struct.unpack('<Q', read(int(method[1], 16), 8))[0] == base + int(method[2], 16)
assert all(x['old_site_matches'] for x in inventory['preimages'].values())
print(json.dumps({'sha256': inventory['sha256'], 'guardByteRanges': checks,
                  'pointerLinks': 6, 'hookPreimages': 'verified', 'runtimeValidated': False}))
