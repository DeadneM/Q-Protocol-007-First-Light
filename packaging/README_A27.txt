Q Protocol A27 - x64 Weapon Graph Scan TEST
=============================================

EXPERIMENTAL: A26 DXGI queue capture + A27 weapon scanner correction.

WHY THIS EXISTS
---------------
A user reported 0 weapon graphs at player loadout anchor 0x80B60E50.
The A23 scanner previously skipped player-local scans above 0x80000000.
A27 now checks real Windows x64 user address limits and scans a bounded
+/-32 MiB area around committed/readable player loadout memory.

GPU DEVICE_LOST remains OPEN: this build does NOT claim a GPU crash fix.
The existing A26 DXGI queue capture and A25 overlay are retained.

UNCHANGED
---------
F1-F12, License To Kill, ammo, manual/automatic loadouts, weapon IDs,
gameplay hook, A24 safety, overlay UI, native DXGI and QProtocol.ini.

INSTALLATION
------------
Back up your current QProtocol.ini and any game-local dxgi.dll first.
Extract the following four files directly into the Retail folder,
next to 007FirstLight.exe, e.g.:

C:\Program Files (x86)\Steam\steamapps\common\007 First Light\Retail\

dxgi.dll
QProtocol.asi
QProtocol.ini
README.txt

The ASI loader dxgi.dll is included; do not install two DXGI proxies
into the same game directory. Do NOT replace the Windows System32 DLL.

DIAGNOSTICS
-----------
Please send QProtocol.log after trying F3 in-game.
Look for 'A27 graph scan:' showing the player anchor, local range,
memory scanning statistics, and the follow-up 'A23 graph index:' line.
Provide QProtocolDXGI.log as well for startup/loader issues.

If the game crashes, restore stable v0.9.6 and back up this TEST log.
