Q Protocol v0.9.3 - Fresh Core A20I
========================================

COMPATIBILITY
-------------
For the October 7, 2026 version of 007 First Light.

MAIN CHANGES
------------
- New Weapons tab for configuring Weapon 1-8 / F5-F12.
- Previous weapon catalogue, Spawn Weapon and Runtime Discovery tools moved to Debug.
- F1 now reads the native License To Kill state calculated by the game and forces the opposite state:
  - native OFF -> FORCE ON
  - native ON  -> FORCE OFF
- Keeps the validated A20F direct native ItemEntry/Spawner weapon system.

NOTE
----
Debug -> Spawn Weapon is kept as a diagnostic tool. Story-mode gameplay state can
still interfere with equipping a diagnostic spawn; this does not affect the normal
F5-F12 configurable weapon slots.

DEFAULT CONTROLS
----------------
F1  Toggle License To Kill from the current native state
F2  Add ammunition
F3  Manual loadout
F4  Q-Pistol swap
F5-F12  Configurable weapon slots
Insert  Overlay

DEFAULT AUTO PROFILE
--------------------
AUTO is disabled by default.
QPistol=QPistolSilenced
OneHanded=None
TwoHanded=None

FILES
-----
QProtocol.asi
QProtocol.ini
README.txt
