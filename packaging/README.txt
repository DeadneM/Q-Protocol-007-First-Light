Q Protocol v0.9.4 - Fresh Core A20J Clean Core
==================================================

BASE
----
Validated A20J Clean Core, built from the validated v0.9.3 / A20I gameplay base.

CHANGES
-------
- Removed obsolete donor/pair-clone restoration code.
- Removed dead core state and dead License To Kill parser state.
- Removed unused overlay telemetry and stale historical naming.
- Simplified OverlayPump to the state it actually consumes.
- Debug catalogue Spawn Weapon now reuses the shared overlay spawn helper.
- Q Protocol's own WriteProcessMemory import is removed.
- ReadProcessMemory and VirtualProtect remain because the current native hooks require them.

VALIDATED
---------
In-game validation passed for:
- F1 native-state License To Kill toggle
- F2 ammo
- F3 Manual loadout
- F4 Q-Pistol swap
- tested F5-F12 weapon slots
- AUTO
- PLAYER NOT READY -> READY reset/restart
- Loadout / Weapons / Debug / Hotkeys overlay
- repeated Debug Spawn Weapon native calls

UNCHANGED
---------
- 31-weapon catalogue and all RIDs
- native ItemEntry/Spawner weapon architecture
- weapon timings and retry policy
- AUTO sequencing
- public INI defaults
- Runtime Discovery

PACKAGE
-------
The release ZIP contains these files directly at its root:
- QProtocol.asi
- QProtocol.ini
- README.txt
