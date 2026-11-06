"""Preserve the reported-working startup while tightening weapon pickup/F4."""
from pathlib import Path
import json
import re
import subprocess

BASE = 'c12ee2de524e7bb2988897f68603205e65d64a3a'

def old(path):
    return subprocess.check_output(['git', 'show', f'{BASE}:{path}'], encoding='utf-8')

for path in ['src/OverlayA19.cpp', 'src/Overlay.h', 'src/GameplayHook.asm', 'src/Updates.cpp',
             'updater/UpdateEngine.cs', 'config/QProtocol.ini', 'src/runtime/NativePlayer.h',
             'src/runtime/PlayableSession.h', 'src/runtime/GameplayMailbox.h', 'src/NativePlayerGuards.inl',
             'src/dxgi_proxy/ProxyDxgi.cpp', 'src/first_debug/imgui_impl_dx12_first_debug.cpp']:
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
             'PublishAmmoProfileGameplayThread', 'ReadDispatchPlayer', 'ObserveWorkerPlayer',
             'InstallGameplayHook', 'InstallLicenseToKillObserveHook', 'LoadConfig', 'QueueWeapon']:
    assert function(core, name) == function(baseline, name), name
inventory = Path('src/runtime/NativeInventory.h').read_text(encoding='utf-8')
for name in ['CaptureSpawnToken', 'ReadSpawnOutput', 'ReadNativeItem', 'ReadAttachment', 'NativeHandleSlot']:
    assert function(inventory, name) == function(old('src/runtime/NativeInventory.h'), name), name
assert 'const NativePickupInput input{request.playerId, item.id, 2};' in core
assert 'g_qpistolSwap.Finish(delivery.spawnTicket, true)' in core
assert 'g_qpistolSwap.Finish(delivery.spawnTicket, false)' in core
assert 'g_qpistolSwap.nextB =' not in core
assert 'if (runtime::CanRetryPickup(*result))' in core
assert 'Version[] = "v0.9.8-beta.3"' in Path('src/Updates.h').read_text()
evidence = json.loads(Path('evidence/nov2026/beta3-instant-pickup.json').read_text())
assert evidence['property'] == 'm_skipPickupAnimations' and evidence['mode'] == 2
assert any(x['memberOffset'] == '0x28' for x in evidence['propertyRecords'])
print('beta.3 scope PASS: beta.2 GPU startup fix, native map/gates, creation tracking, ammo, config and updater preserved; instant native pickup and receipt-based F4 are experimental.')
