from pathlib import Path
import subprocess

base='8722000d859eb9785a8906cae82adc72d35ae1a2'
def old(path):return subprocess.check_output(['git','show',f'{base}:{path}'],encoding='utf-8')
s=Path('src/QProtocol.cpp').read_text(encoding='utf-8')
s='\n'.join(line for line in s.split('\n') if '// A31_DIAG' not in line)
s=s.replace('\n\nbool ResolveNativeLoadout','\nbool ResolveNativeLoadout',1)
s=s.replace('Q Protocol A31 / Operations Readiness DIAGNOSTIC','Q Protocol v0.9.7 / Updater and Readable Menu')
assert s==old('src/QProtocol.cpp'),'Core behavior changed outside tagged diagnostic metadata/logging'
for path in ['src/OverlayA19.cpp','src/Updates.cpp','src/GameplayHook.asm','src/Overlay.h','config/QProtocol.ini','updater/UpdateEngine.cs','src/dxgi_proxy/ProxyDxgi.cpp','src/first_debug/imgui_impl_dx12_first_debug.cpp']:
    assert Path(path).read_text(encoding='utf-8')==old(path),path
s=Path('src/Updates.h').read_text(encoding='utf-8').replace('Build = 31','Build = 30').replace('Version[] = "A31 DIAGNOSTIC"','Version[] = "v0.9.7"')
assert s==old('src/Updates.h')
s=Path('src/PlayerReadinessDiagnostics.inl').read_text(encoding='utf-8')
for forbidden in ['VirtualProtect','WriteProcessMemory','CreateHook','g_nativeSpawn(']:assert forbidden not in s
assert 'SafeRead' in s and '10000' in s
print('A31 scope PASS: observational metadata/logging only; stable native behavior and protections preserved.')
