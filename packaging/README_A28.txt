Q Protocol v0.9.6 / A28 November Remap TEST

EXPERIMENTAL. Static native-map identification and compatibility gates are
implemented. This package has NOT been validated in game by the maintainer.

Target: Steam build 26252352, installed update observed October 30, 2026.
007FirstLight.exe SHA-256:
6e63df4d2891ea085a71ef9a6f4d24b9db52144879945e8cc1a2e8106fabd3f1
PE TimeDateStamp 0x6AE0DF3C; SizeOfImage 0x06D31000.
Other executable versions are unsupported. Never bypass the compatibility gate.

Base: validated A26/A27 integration, commit 06245ba (PR #7).
Changes: native resolver/type links, LTK, graph vtables, spawn/ammo helpers and
gameplay hook addresses remapped individually. Added native code/type checks
before installing hooks. A28 identity is explicit in QProtocol.log.
Preserved: A25 overlay, A26 authoritative DX12 queue capture, A27 bounded x64
scanner, A24 request guards, F1-F12, AUTO, defaults and the complete INI.
GPU DEVICE_LOST 0x887A002B on another configuration remains unresolved.

INSTALL / ROLLBACK (game closed):
Back up existing dxgi.dll, QProtocol.asi, QProtocol.ini and README.txt together.
Extract this ZIP directly beside Retail/007FirstLight.exe (no parent folder).
Keep your personal QProtocol.ini; the included INI supplies unchanged defaults.
Do not overwrite ReShade/Special K or another DXGI loader without a backup.
Restore all backed-up files to roll back; older mod builds still require their
matching executable. This package does not modify the executable on disk.

MAINTAINER TEST, one session only after agreeing to launch:
1. Check A28 identity, native-map PASS and player READY in QProtocol.log.
2. Test F1 OFF/ON and F2 ammo for the six classes; inspect actual game results.
3. Test one F3 loadout (three roles), F4 and configured F5-F12 slots.
4. Check graph counts and coverage; prior 41/42/43 counts are a reference,
   not an acceptance requirement for a changed level or session.
5. Check AUTO once, overlay Insert, mouse, resize, Save/Reload and player reload.
Stop immediately on crash/unexpected behavior; retain both logs and restore
the backup. Do not repeat a failed runtime experiment.

No public compatibility claim until maintainer acceptance and log review.
