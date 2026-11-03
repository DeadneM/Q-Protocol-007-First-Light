from pathlib import Path
import subprocess

base='8722000d859eb9785a8906cae82adc72d35ae1a2'
def old(p):return subprocess.check_output(['git','show',f'{base}:{p}'],encoding='utf-8')
s=Path('src/QProtocol.cpp').read_text(encoding='utf-8')
s='\n'.join(x for x in s.split('\n') if '// A32_CHANGE' not in x)
s=s.replace('\n\n// A28 compatibility gate:','\n// A28 compatibility gate:',1)
s=s.replace('bool ResolveNativeLoadout(void** outLoadout, bool* emptyNativeHandle = nullptr)', 'bool ResolveNativeLoadout(void** outLoadout)')
s=s.replace('ResolveNativeLoadout(&loadout, &emptyNativeHandle)','ResolveNativeLoadout(&loadout)')
s=s.replace('Q Protocol A33 / Tac Sim Identity Contract TEST','Q Protocol v0.9.7 / Updater and Readable Menu')
assert s==old('src/QProtocol.cpp'),'Unexpected core changes outside empty-handle fallback and guarded native helper'
for p in ['src/OverlayA19.cpp','src/Updates.cpp','src/GameplayHook.asm','src/Overlay.h','config/QProtocol.ini','updater/UpdateEngine.cs','src/dxgi_proxy/ProxyDxgi.cpp','src/first_debug/imgui_impl_dx12_first_debug.cpp']:
 assert Path(p).read_text(encoding='utf-8')==old(p),p
s=Path('src/Updates.h').read_text(encoding='utf-8').replace('Build = 33','Build = 30').replace('Version[] = "A33 TEST"','Version[] = "v0.9.7"');assert s==old('src/Updates.h')
s=Path('src/MissionLoadoutResolver.inl').read_text(encoding='utf-8')
for token in ['resolve(before.spawnerId,reference)','before.root != after.root','before.node != after.node','before.playerId != after.playerId','before.spawnerId != after.spawnerId','vtable != g_exeBase + kPlayerLoadoutVtableRva','memcmp(current,kMissionResolverPreimage,sizeof(current))']:
 assert token in s,token
assert 'resolvedPlayerId = before.playerId;' in s
assert 'SafeRead(identity.node + 0x34,identity.spawnerId)' in s
assert 'identity.spawnerId != invalidSpawnerId' in s
for token in ['VirtualProtect','WriteProcessMemory','CreateHook']:
 assert token not in s,token
print('A33 scope PASS: separate runtime/spawner IDs; empty-handle fallback only; stable gameplay, renderer, input, INI and updater preserved.')
