"""Keep accepted beta.3 manual delivery and graphics intact during AUTO repair."""
from pathlib import Path
import re
import subprocess

BASE = '2f0ef7054268fda81bb31c70839899f8e42897dd'

def old(path):
    return subprocess.check_output(['git', 'show', f'{BASE}:{path}'], encoding='utf-8')

for path in ['src/OverlayA19.cpp', 'src/Overlay.h', 'src/GameplayHook.asm', 'src/Updates.cpp',
             'updater/UpdateEngine.cs', 'config/QProtocol.ini', 'src/runtime/NativePlayer.h',
             'src/runtime/PlayableSession.h', 'src/runtime/GameplayMailbox.h', 'src/runtime/NativeInventory.h',
             'src/runtime/WeaponSwap.h', 'src/NativePlayerGuards.inl', 'src/dxgi_proxy/ProxyDxgi.cpp',
             'src/first_debug/imgui_impl_dx12_first_debug.cpp']:
    assert Path(path).read_text(encoding='utf-8') == old(path), path

def function(source, name):
    match = re.search(r'\b' + re.escape(name) + r'\s*\(', source)
    start = source.index('{', match.start())
    lexical = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/')
    clean = lexical.sub(lambda m: ' ' * len(m.group()), source)
    depth = 0
    for at in range(start, len(clean)):
        if clean[at] == '{': depth += 1
        elif clean[at] == '}':
            depth -= 1
            if depth == 0: return source[start:at+1]
    raise AssertionError(name)

core = Path('src/QProtocol.cpp').read_text(encoding='utf-8')
baseline = old('src/QProtocol.cpp')
for name in ['ValidateNovemberNativeMap', 'BuildGraphIndex', 'ValidateGraph', 'ExecuteSpawn',
             'PublishAmmoProfileGameplayThread', 'PublishPickupGameplayThread',
             'ReadDispatchPlayer', 'ObserveWorkerPlayer', 'InstallGameplayHook',
             'InstallLicenseToKillObserveHook', 'LoadConfig', 'QueueWeapon', 'StartLoadoutPackage',
             'FinishActiveWeapon', 'ProcessWeaponQueue']:
    assert function(core, name) == function(baseline, name), name
old_delivery = old('src/NativeDeliveryGuards.inl')
delivery = Path('src/NativeDeliveryGuards.inl').read_text(encoding='utf-8')
assert delivery.startswith(old_delivery[:old_delivery.rindex('    return true;')])
assert 'packageOwned && g_loadoutPackage.source == LoadoutSource::Auto' in core
assert 'if (command->payload.autoOwned && result.invoked && result.token.record)' in core
assert 'ReadAttachment' in function(core, 'OwnsAutoSpawnCompletion')
assert 'ReadDispatchPlayer' in function(core, 'OwnsAutoSpawnCompletion')
assert 'g_ownedAutoSpawns.Clear();' in function(core, 'ResetWeaponRuntime')
asm = Path('src/AutoSpawnHook.asm').read_text()
assert asm.count('jmp qword ptr [g_qpAutoSpawnCallTarget]') == 2
assert 'lea rax, [rax-28h]' in asm and 'call qword ptr' not in asm
assert 'Version[] = "v0.9.8-beta.4"' in Path('src/Updates.h').read_text()
print('beta.4 scope PASS: beta.3 manual pickup/F4, GPU, map, native spawn, ammo, player/session, INI and updater preserved; only tracked AUTO completion can select native placement.')
