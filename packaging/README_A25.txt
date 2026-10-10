Q Protocol v0.9.5 - A25 Overlay Default Layout TEST
=====================================================

This is a small UI-only compatibility test built strictly from the
validated v0.9.5 / A24 source. No gameplay changes or INI changes.

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
Exactly these three files at the ZIP root:
QProtocol.asi
QProtocol.ini
README.txt

INSTALL
-------
Back up your configured QProtocol.ini first. Replace the QProtocol.asi
and optionally the INI in your usual Q Protocol installation location.
This archive does not include a dxgi.dll loader. It works with the
same ASI loading method as public v0.9.5, or with the previously
supplied custom DXGI proxy test if that is separately installed.

TEST POINTS
-----------
1. Open with Insert at native resolution and verify Save, Reload and
   Defaults are visible without scrolling.
2. Check Loadout, Weapons, Debug, Hotkeys tabs.
3. Resize to smaller dimensions. The tab contents may scroll, but
   the three action buttons must remain visible.
4. Check Insert close/reopen, Alt-Tab and weapon hotkeys.
5. Send QProtocol.log if errors occur.

This is a TEST build; public v0.9.5 remains unchanged.
