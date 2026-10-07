Q Protocol - Fresh Core A20G Mod Weapon Slots TEST
=====================================================

BASE
----
Strictly based on validated public v0.9.2 / Fresh Core A20F.

TESTED CHANGE
-------------
Adds a new "Mod" overlay tab for configuring the eight weapon hotkey slots:

Weapon 1 = F5
Weapon 2 = F6
Weapon 3 = F7
Weapon 4 = F8
Weapon 5 = F9
Weapon 6 = F10
Weapon 7 = F11
Weapon 8 = F12

Each slot uses the same validated weapon catalogue and visibility rules as the
Manual/Automatic loadout selectors. Experimental weapons can be shown with the
existing checkbox; Not Working entries remain hidden.

The selections are saved directly to [Hotkey_F5] through [Hotkey_F12] in
QProtocol.ini. Press Save in the overlay to apply the new assignments.

GAMEPLAY
--------
No A20F gameplay primitive is changed.
Direct native ItemEntry/Spawner weapon spawning remains untouched.
Manual loadout, AUTO, ammo and License To Kill remain untouched.

NOTE
----
The Weapons -> Spawn Weapon button remains a diagnostic single-spawn tool.
This build does not assume that Story mode accepts that path in every context.

FILES
-----
QProtocol.asi
QProtocol.ini
README.txt
