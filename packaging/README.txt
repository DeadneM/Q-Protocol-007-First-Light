Q Protocol - Fresh Core A20L Native Melee Policy TEST
======================================================

BASE
----
Validated public v0.9.4 / Fresh Core A20J Clean Core
plus the A20K three-state Rules of Engagement cycle.

A20L CHANGE
-----------
A20K proved that the three F1 states cycle correctly, but the middle
LICENSE TO PUNCH state did not actually restore punching.

A20L now controls the game's real humanoid melee-disable property:

DisableMeleeAttack

Current October executable mapping:
- local humanoid EntityRef getter: EXE+0x015D4210
- humanoid lookup registry:        EXE+0x06784048
- DisableMeleeAttack setter:       EXE+0x016DD1E0
- DisableMeleeAttack getter:       EXE+0x016DD210

The native property is bit 0x08 of ZHumanoid + 0xC4.

F1 POLICY
---------
OFF
- lethal force disabled
- DisableMeleeAttack = ON
- Bond should not initiate punching

LICENSE TO PUNCH
- lethal force disabled
- DisableMeleeAttack = OFF
- punching should work

LICENSE TO KILL
- lethal force enabled
- DisableMeleeAttack = OFF
- punching remains available

ROBUSTNESS
----------
- The original melee value is captured for the current local humanoid.
- The requested melee policy is verified through the native getter.
- While an F1 override is active, the policy is checked every 100 ms so the
  game cannot silently overwrite the requested state.
- The captured original melee value is restored on DLL shutdown when possible.
- A new player/humanoid object gets a fresh capture.

UNCHANGED
---------
- A20J Clean Core weapon architecture
- F2 ammo
- F3 Manual loadout
- F4 Q-Pistol swap
- F5-F12 weapon slots
- AUTO
- all weapon RIDs
- weapon timings / retries
- Weapons / Debug overlay behavior

TEST
----
Please cycle F1 and test each state:

1. OFF
   - punching must be blocked

2. LICENSE TO PUNCH
   - punching must work
   - lethal firearm permission must remain off

3. LICENSE TO KILL
   - lethal firearm permission must work
   - punching should still work

4. Back to OFF
   - punching must be blocked again

Please send QProtocol.log after testing.

FILES
-----
QProtocol.asi
QProtocol.ini
README.txt
