Q Protocol v0.9.6 - A25 Accessible Overlay + Native DXGI
=========================================================

Public cumulative v0.9.6 based on v0.9.5 / A24 gameplay.
A25 updates only the ImGui window layout; the ZIP also ships our native
x64 DXGI proxy by default. Weapon IDs, F1-F12, AUTO and INI are unchanged.

CHANGES
-------
- Default window size on first open increases from 900 x 650 to
  1000 x 850 ImGui units, clamped to the real viewport work area.
- The lower Save / Reload / Defaults actions are always visible,
  outside the scrollable tab content region.
- On smaller screens, the central content can scroll while the
  action buttons stay accessible at the bottom.
- The floating window remains movable, resizable and closable.
- Mouse capture and DX12 rendering remain identical to v0.9.5.

GAMEPLAY
--------
F1 LTK, F2 Ammo, F3 manual loadout, F4 Q-Pistol, F5-F12 weapon
hotkeys, AUTO, the A23 native graph scanner, and A24 duplicate/burst
safeguards are unchanged.

CONTENTS
--------
All next builds now include the custom DXGI loader by default.
Exactly these FOUR files at the ZIP root:
dxgi.dll
QProtocol.asi
QProtocol.ini
README.txt

INSTALL
-------
Back up your configured QProtocol.ini before replacing it.
Place dxgi.dll, QProtocol.asi and the INI in the game folder alongside
007FirstLight.exe.

IMPORTANT: If a different dxgi.dll is already in that folder, back it up.
Do not overwrite ReShade, Special K or another DXGI proxy without restoring
a compatible chain. Never replace Windows/System32/dxgi.dll.
Our dxgi.dll forwards native DXGI calls to the real Windows system DLL
and loads the bundled QProtocol.asi outside DllMain.
QProtocolDXGI.log confirms its startup and QProtocol.log covers gameplay.

If you already installed our same native custom dxgi.dll, you may replace
it with this cumulative build or keep your existing one after backing it up.
The public v0.9.5 release remains available for rollback.

POST-INSTALL CHECKS
-------------------
1. Open with Insert at native resolution and verify Save, Reload and
   Defaults are visible without scrolling.
2. Check Loadout, Weapons, Debug, Hotkeys tabs.
3. Resize to smaller dimensions. The tab contents may scroll, but
   the three action buttons must remain visible.
4. Check Insert close/reopen, Alt-Tab and weapon hotkeys.
5. If issues occur, collect QProtocolDXGI.log and QProtocol.log.

This is the v0.9.6 public release, promoted after user acceptance.
Compatibility with every Windows build/overlay/GPU combination is not guaranteed.
Do not overwrite a different dxgi.dll without first making a backup.
