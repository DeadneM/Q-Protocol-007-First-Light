Q Protocol - Fresh Core A20J Clean Core TEST
=============================================

BASE
----
Strictly based on validated public v0.9.3 / Fresh Core A20I.

PURPOSE
-------
Cleanup-only candidate. No weapon RID, gameplay primitive, timing, retry,
AUTO behavior or public default is intentionally changed.

REMOVED DEAD CORE
-----------------
- obsolete donor/pair-clone constant and restoration branch
- SafeWrite / Q Protocol's own WriteProcessMemory path
- dead donor descriptor fields
- dead directGraph / seenBusy state
- dead LtkState / ReadLicenseToKillState parser
- write-only gameplay-hook-installed flag

OVERLAY CLEANUP
---------------
- removed unused autoDone / queueCount / Q-Pistol telemetry copies
- simplified OverlayPump to player-ready input only
- removed unused hooks-installed flag
- renamed internal Mod weapon-slot state to Weapons naming
- renamed the old weapon-catalog container internally to Debug
- catalog Spawn Weapon now reuses the same QueueOverlaySpawn helper as Runtime Discovery
- removed stale A19/A5 status labels

UNCHANGED
---------
- F1 native-state License To Kill toggle
- F2 ammo
- F3 Manual loadout
- F4 Q-Pistol swap
- F5-F12 configurable weapon slots
- AUTO sequencing
- direct native ItemEntry/Spawner weapon spawning
- 31-weapon catalogue and all RIDs
- Runtime Discovery
- Debug tools
- QProtocol.ini defaults

TEST
----
Please verify F1, F2, F3, F4, several F5-F12 slots, AUTO and the overlay tabs:
Loadout / Weapons / Debug / Hotkeys.

FILES
-----
QProtocol.asi
QProtocol.ini
README.txt
