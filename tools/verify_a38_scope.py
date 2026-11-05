"""Restrict A38 to the reviewed executable map, guards and build identity."""
from pathlib import Path
import json
import re
import subprocess

BASE = 'ea5073cd83c9506c6d2f73e058da06d6f7de889a'
report = json.loads(Path('evidence/nov2026/a38-native-map.json').read_text(encoding='utf-8'))
mapping = {int(a, 16): int(b, 16) for a, b in report['mapping'].items()}

def old(path):
    return subprocess.check_output(['git', 'show', f'{BASE}:{path}'], encoding='utf-8')

def remap(source):
    return re.sub(r'0x[0-9A-Fa-f]+', lambda m: f'0x{mapping[int(m[0], 16)]:08X}'
                  if int(m[0], 16) in mapping else m[0], source)

allowed = {'src/QProtocol.cpp', 'src/NativePlayerGuards.inl', 'src/runtime/NativePlayer.h',
           'src/Updates.h', 'tests/NativePlayerTests.cpp'}
paths = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', BASE, '--',
                                 'src', 'config', 'updater', 'tests'], encoding='utf-8').splitlines()
for path in paths:
    if path not in allowed:
        assert Path(path).read_text(encoding='utf-8') == old(path), path

for path in allowed:
    expected = old(path)
    if path in report['guards']:
        for name, raw in report['guards'][path].items():
            pattern = r'(const BYTE ' + name + r'\[\] = \{)[^}]+'
            expected, count = re.subn(pattern, lambda m: m[1] + raw, expected)
            assert count == 1, (path, name)
    expected = remap(expected)
    if path == 'src/QProtocol.cpp':
        expected = expected.replace(
            'A28 TEST: Steam build 26252352, SHA-256 ' + report['oldExeSha256'] + '.',
            'A38 TEST: Steam build 26430236, SHA-256 ' + report['exeSha256'] + '.')
        expected = expected.replace('Q Protocol A37 / Weapon Cadence TEST',
                                    'Q Protocol A38 / November 5 Remap TEST')
    if path == 'src/Updates.h':
        expected = expected.replace('Build = 37', 'Build = 38').replace('A37 TEST', 'A38 TEST')
    assert Path(path).read_text(encoding='utf-8') == expected, path

assert report['exeSha256'] == 'f92c0e8600aa3b692a437f2c0795068f509db1a73a4794b619e6d60bd5d35c43'
assert report['timestamp'] == '0x6aeafdda' and report['sizeOfImage'] == '0x6d32000'
print('A38 scope PASS: only reviewed native addresses, guard preimages and build identity differ from A37. Gameplay validation pending.')
