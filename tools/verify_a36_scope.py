"""Preserve accepted native primitives/UI/config while replacing their lifecycle."""
from pathlib import Path
import re
import subprocess

BASE = '8722000d859eb9785a8906cae82adc72d35ae1a2'
def old(path):
    return subprocess.check_output(['git','show',f'{BASE}:{path}'],encoding='utf-8')
for path in ['src/OverlayA19.cpp','src/Overlay.h','src/GameplayHook.asm',
             'src/Updates.cpp','updater/UpdateEngine.cs','config/QProtocol.ini',
             'src/dxgi_proxy/ProxyDxgi.cpp','src/first_debug/imgui_impl_dx12_first_debug.cpp']:
    assert Path(path).read_text(encoding='utf-8') == old(path),path

def function(source,name):
    match=re.search(r'\b'+re.escape(name)+r'\s*\(',source)
    assert match,name
    start=source.index('{',match.start())
    # Strip strings/comments for brace depth, keeping source offsets unchanged.
    lexical=re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/')
    clean=lexical.sub(lambda m:' '*len(m.group()),source)
    depth=0
    for at in range(start,len(clean)):
        if clean[at]=='{':depth+=1
        elif clean[at]=='}':
            depth-=1
            if depth==0:return source[start:at+1]
    raise AssertionError(name)

core=Path('src/QProtocol.cpp').read_text(encoding='utf-8')
baseline=old('src/QProtocol.cpp')
for name in ['WriteCodeBytes','ValidateNovemberNativeMap','InstallGameplayHook',
             'InstallLicenseToKillObserveHook','RemoveLicenseToKillObserveHook',
             'ApplyLicenseToKillForce','RestoreLicenseToKillNatural','ToggleLicenseToKill',
             'LoadConfig','BuildGraphIndex','ValidateGraph','ResolveGraph','IsSpawnerIdle',
             'QueueWeapon','StartLoadoutPackage','RetryLoadoutPackage','RetryUnitaryWeapon']:
    assert function(core,name)==function(baseline,name),name
for name in ['kNativeSpawnRva','kNativeAmmoInsertRva','kNativeAmmoNotifyRva','kGameplayHookRva',
             'kAmmoNotifyContextOffset','kAmmoInputVectorOffset','kAmmoLockOffset','kAmmoNotifyEventId']:
    pattern=r'\b'+name+r'\s*=\s*([^;]+);'
    assert re.search(pattern,core).group(1)==re.search(pattern,baseline).group(1),name
for gone in ['ResolveNativeLoadout(','ResolveMissionLoadout(','g_pendingAmmo','g_ammoPending',
             'g_pendingSpawner','autoStarted','autoDone','MissionLoadoutResolver.inl']:
    assert gone not in core,gone
assert 'snapshot.Fresh(GetTickCount64())' in core
assert 'ValidateNovemberNativeMap() || !ValidateNativePlayerMap()' in core
assert 'g_spawnMailbox.Complete(*command, qp::g_tickPlayer.snapshot, result)' in core
assert 'Log(' not in function(core,'PublishAmmoProfileGameplayThread')
provider=Path('src/runtime/NativePlayer.h').read_text(encoding='utf-8')
assert 'ResolveLocalPlayer' not in provider
assert 'out.observation.gameplayAllowed = alive && !inSequence' in provider
assert 'out.loadout = loadout.pointer' in provider
assert 'read(binding.component + 0x200, spawnerId)' in provider
assert 'identity.player' not in function(core,'PublishAmmoProfileGameplayThread')
header=Path('src/Updates.h').read_text(encoding='utf-8')
assert header.replace('Build = 36','Build = 30').replace('"A36 TEST"','"v0.9.7"')==old('src/Updates.h')
print('A36 scope PASS: stable primitives/RIDs/config/UI/DX12/updater preserved; shared native-character/session/mailbox lifecycle replaces loadout readiness.')

assert 'return ObserveWorkerPlayer();' in function(core,'ResolvePlayer')
assert 'ObserveWorkerPlayer' not in core[core.index('extern "C" void QpGameplayTick() {'):]
assert 'qp::ReadDispatchPlayer()' in core[core.index('extern "C" void QpGameplayTick() {'):]
assert 'FailActiveWeapon("gameplay trigger timeout")' not in core
assert 'out.observation.confirmedDead = !alive;' in provider
assert 'Mask = 0xFFFFFFFF' not in core

assert '!ValidateNativeDeliveryMap()' in core
assert 'result.invoked = true;' in function(core,'ExecuteSpawn')
assert 'runtime::CaptureSpawnToken' in function(core,'ExecuteSpawn')
assert 'Attachment::Present' in function(core,'ProcessWeaponQueue')
assert 'nativeEntered' in function(core,'FailActiveWeapon')
assert 'Log(' not in function(core,'PublishPickupGameplayThread')
assert 'PublishPickupGameplayThread' not in function(core,'WorkerThread')
assert 'qp::g_pickupMailbox.Complete' in core
assert 'NativePickupInput input{request.playerId, item.id, 0}' in core
