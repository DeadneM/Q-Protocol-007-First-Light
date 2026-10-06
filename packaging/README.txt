Q Protocol v0.9.1 - Fresh Core A19 Audit Hardening
====================================================

PUBLIC RELEASE BUILD

This release promotes the fully audited Fresh Core A19 line.

Highlights
----------
- Remappable overlay invocation key through [Overlay] ToggleKey.
- Overlay toggle safely coexists with F1-F12, including when remapped to them.
- OneHanded and TwoHanded support Off / None.
- Automatic defaults are QPistolSilenced + Off + Off.
- Experimental weapons remain hidden from Manual/Auto selectors by default.
- Latest user-validated weapon statuses are included.
- Full catalogue/config audit runs before every build.
- DX12 overlay bootstrap remains fail-open.

Gameplay foundation
-------------------
Fresh Core A5 remains the validated gameplay primitive base.

A19 does NOT alter:
- ResolvePlayer
- GiveWeapon
- AddAmmo
- weapon RIDs
- ammo classes
- 500 ms weapon stabilization
- native gameplay hook semantics

Current hotkeys
---------------
F1  License To Kill
F2  Manual Ammo
F3  Manual Loadout
F4  Q-Pistol Swap
F5-F12 Weapon slots

Generic F1-F12 Action reassignment is not implemented yet.

Installation
------------
Copy into the game directory:
- QProtocol.asi
- QProtocol.ini
- README.txt

Default overlay key: Insert
It can be remapped from the Hotkeys tab.
