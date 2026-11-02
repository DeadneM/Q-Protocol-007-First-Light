"""Audit preservation of integrated A26/A27 and A28 compatibility checks."""
import json
import re
import subprocess
from pathlib import Path

baseline = '06245ba6623f590d165c05d227c078f2072831e5'
def old(path):
    return subprocess.check_output(['git', 'show', f'{baseline}:{path}'], encoding='utf-8')

path = 'src/QProtocol.cpp'
original = old(path)
updated = Path(path).read_text(encoding='utf-8')
manifest = json.loads(Path('evidence/nov2026/a28-native-map.json').read_text(encoding='utf-8'))
normalized = updated
for name, value in {**manifest['map'], 'kExpectedTimeDateStamp': manifest['timestamp']}.items():
    pattern = rf'(constexpr (?:std::uintptr_t|DWORD) {name}\s*=\s*)(0x[0-9A-Fa-f]+)'
    current = re.search(pattern, updated)
    previous = re.search(pattern, original)
    assert current and previous and int(current[2], 16) == int(value, 16), name
    normalized = re.sub(pattern, lambda m: m[1] + previous[2], normalized)

start = '// A28 compatibility gate:'
end = 'bool ValidateTargetExecutable() {'
assert updated.count(start) == 1
guard = updated[updated.index(start):updated.index(end)]
assert 'memcmp(current, expected, size) != 0' in guard
assert 'got != size' in guard
assert 'value != g_exeBase + link[1]' in guard
for check in manifest['checks']:
    assert f'"{check["name"]}", {check["rva"]}' in guard
    expected = ', '.join('0x' + b.upper() for b in check['bytes'].split())
    assert '{' + expected + '}' in guard
normalized = normalized[:normalized.index(start)] + normalized[normalized.index(end):]
line = '    if (!ValidateNovemberNativeMap()) return false;\n\n'
assert normalized.count(line) == 1
normalized = normalized.replace(line, '')
normalized = re.sub(r'// A28 TEST: Steam build[^\n]*', '// October 2026 executable mappings.', normalized)
normalized = normalized.replace('Q Protocol v0.9.6 / A28 November Remap TEST',
                                'Q Protocol v0.9.4 / Fresh Core A20J Clean Core')
normalized = normalized.replace('Scope: A28 TEST static native remap; A25/A26/A27 preserved; runtime validation pending.',
                                'Scope: validated A20J Clean Core; A20I gameplay/RIDs/timings preserved.')
assert normalized == original, 'Gameplay changed outside the permitted native map, gate and build identity'
assert updated.index('if (!ValidateNovemberNativeMap()) return false;') < updated.index('    g_nativeSpawn = reinterpret_cast<NativeSpawnFn>')
for path in ('src/OverlayA19.cpp', 'src/GameplayHook.asm', 'src/Overlay.h', 'config/QProtocol.ini',
             'src/first_debug/imgui_impl_dx12_first_debug.cpp',
             'src/first_debug/imgui_ps.hlsl', 'src/first_debug/imgui_vs.hlsl',
             'src/dxgi_proxy/ProxyDxgi.cpp', 'src/dxgi_proxy/DxgiSmoke.cpp'):
    assert Path(path).read_text(encoding='utf-8') == old(path), f'Protected baseline changed: {path}'
print('A28 scope PASS: A26 renderer, A27 scanner, A25 UI, assembly, gameplay, DXGI and INI preserved.')
print('Static checks do not establish in-game compatibility or fix GPU DEVICE_LOST.')
