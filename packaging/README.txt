Q Protocol - Fresh Core A20M Close Combat Input Probe TEST
==========================================================

BASE
----
Validated public v0.9.4 / Fresh Core A20J Clean Core
plus the A20K three-state Rules of Engagement cycle.

WHY A20M
--------
A20L proved that ZHumanoid::DisableMeleeAttack is not the authoritative
permission for Bond to enter close combat. TacSim can still prohibit punching
independently of that humanoid property.

A20M therefore moves one layer higher and observes the actual gameplay input
override system.

THIS BUILD IS READ-ONLY FOR CLOSE-COMBAT INPUT
----------------------------------------------
A20M does NOT yet force or remove an input override.

It discovers and logs live instances of:
- ZGameplayInputOverrideManager
- ZInputConfigHumanoid
- ZCLBlockPlayerInputAction
- ZCLBlockHumanoidPlayerCloseCombatInput
- ZCLUnblockHumanoidPlayerCloseCombatInput

For ZInputConfigHumanoid it records the native TEntityRef fields for:
- QuickMeleeAttack
- ChargedMeleeAttack
- CloseCombatSidestep
- Grab
- Parry
- Shoot

For ZCLBlockPlayerInputAction it records:
- state bytes +0x60 / +0x61 / +0x62
- Input / Target / Runtime entity-reference fields

F1
--
The A20K three-state cycle remains unchanged:

OFF -> LICENSE TO PUNCH -> LICENSE TO KILL -> OFF

A20M takes a delayed input-override snapshot after PLAYER READY and after each
successful F1 transition.

TEST PLAN
---------
Best comparison:

1. Start a normal mission where Bond can punch.
2. Wait until fully playable.
3. Press F1 through OFF / LICENSE TO PUNCH / LICENSE TO KILL.
4. Exit and save QProtocol.log.

Then:

5. Start TacSim, where punching is normally unavailable.
6. Repeat the same F1 cycle.
7. Send the second QProtocol.log.

The differences between those two logs should identify the authoritative
close-combat blocker before we make A20N actively control it.

UNCHANGED
---------
- A20J weapon architecture
- F2 ammo
- F3 Manual loadout
- F4 Q-Pistol swap
- F5-F12 weapon slots
- AUTO
- all weapon RIDs / timings / retries
- Runtime Discovery / Debug
- public INI defaults

FILES
-----
QProtocol.asi
QProtocol.ini
README.txt
