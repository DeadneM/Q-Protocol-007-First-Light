Q Protocol - Fresh Core A20H Weapons/Debug + LTK Force Toggle TEST
===================================================================

BASE
----
Strictly based on validated v0.9.2 / A20F plus validated A20G Weapons-slot UI.

CHANGES
-------
- Renames the A20G "Mod" tab to "Weapons".
- Renames the previous weapon catalogue/spawn tab to "Debug".
- F1 no longer alternates Force ON -> Natural.
- F1 now alternates Force ON <-> Force OFF after the first press.

LICENSE TO KILL
---------------
The game has its own native dynamic License To Kill logic.

Natural code paths:
- native false path: XOR DL,DL
- native true path:  MOV DL,1

A20H controls both paths:
- FORCE ON  = both paths resolve to true.
- FORCE OFF = both paths resolve to false.
- DLL shutdown restores both original native instructions.

Important: immediately after game start the mod is in Natural mode, so the very
first F1 press forces ON. After that F1 alternates FORCE ON / FORCE OFF.

WEAPONS TAB
-----------
Weapon 1 = F5
Weapon 2 = F6
...
Weapon 8 = F12

The old catalogue / Spawn Weapon / Runtime Discovery area is now under Debug.

FILES
-----
QProtocol.asi
QProtocol.ini
README.txt
