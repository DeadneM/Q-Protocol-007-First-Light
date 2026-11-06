"""Guard the delivery repair against unrelated changes to the accepted primitives."""
from pathlib import Path
import re
import subprocess

BASE = '5bc3ac2d64567328b6ab079bb79599ba890cf5ec'

def old(path):
    return subprocess.check_output(['git', 'show', f'{BASE}:{path}'], encoding='utf-8')

for path in ['src/GameplayHook.asm', 'src/Overlay.h', 'src/Updates.cpp', 'updater/UpdateEngine.cs',
             'src/dxgi_proxy/ProxyDxgi.cpp', 'src/first_debug/imgui_impl_dx12_first_debug.cpp',
             'config/QProtocol.ini', 'src/runtime/NativePlayer.h', 'src/runtime/PlayableSession.h',
             'src/runtime/GameplayMailbox.h', 'src/NativePlayerGuards.inl', 'tests/NativePlayerTests.cpp']:
    assert Path(path).read_text(encoding='utf-8') == old(path), path
overlay = Path('src/OverlayA19.cpp').read_text(encoding='utf-8')
assert overlay.replace('    ImGui::TextDisabled("Internal build: %d", updates::Build);\n', '') == old('src/OverlayA19.cpp')

def function(source, name):
    match = re.search(r'\b' + re.escape(name) + r'\s*\(', source)
    assert match, name
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
for name in ['ValidateNovemberNativeMap', 'InstallGameplayHook', 'InstallLicenseToKillObserveHook',
             'RemoveLicenseToKillObserveHook', 'ApplyLicenseToKillForce', 'RestoreLicenseToKillNatural',
             'ToggleLicenseToKill', 'LoadConfig', 'ReadDispatchPlayer', 'ObserveWorkerPlayer',
             'IsSpawnerIdle', 'QueueWeapon', 'StartLoadoutPackage', 'PublishAmmoProfileGameplayThread']:
    assert function(core, name) == function(baseline, name), name
assert 'if (idle) {\n            FinishActiveWeapon(true);' in function(core, 'ProcessWeaponQueue')
assert 'g_weaponDeliveries.empty()' in core
assert 'if (!RuntimePlayerMatches(request.playerId)) return false;' in function(core, 'PublishPickupGameplayThread')
assert 'ReadAttachment' in function(core, 'PublishPickupGameplayThread')
assert 'delivery.spawnTicket.Matches(g_workerPlayer.snapshot)' in function(core, 'PollWeaponDelivery')
assert 'delivery.pickupTicket && !delivery.pickupProcessed' in core
assert 'Version[] = "v0.9.8-beta.1"' in Path('src/Updates.h').read_text()
assert 'Build = 39' in Path('src/Updates.h').read_text()
print('Delivery scope PASS: map, player/session gate, gameplay hooks, ammo primitive, input, config and updater preserved. Inventory receipt still requires gameplay acceptance.')
