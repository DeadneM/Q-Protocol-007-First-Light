Q Protocol - Fresh Core A20K Three-State ROE TEST
===================================================

BASE
----
Validated public v0.9.4 / Fresh Core A20J Clean Core.

A20K CHANGE
-----------
F1 now cycles the game's native Rules of Engagement states:

OFF -> LICENSE TO PUNCH -> LICENSE TO KILL -> OFF

The October executable exposes two separate native outputs in the same ROE
calculation:
- isLicenseToPunch
- isLethalForceEnabled

A20K observes and controls both channels together.

EXPECTED BEHAVIOR
-----------------
OFF
- lethal force disabled
- close-combat / punching permission disabled

LICENSE TO PUNCH
- lethal force disabled
- close-combat / punching permission enabled

LICENSE TO KILL
- lethal force enabled

The original game ROE calculation remains underneath the override. The mod only
forces the effective output after observing the native state.

UNCHANGED
---------
- A20J Clean Core weapon spawning
- F2 ammo
- F3 Manual loadout
- F4 Q-Pistol swap
- F5-F12 weapon slots
- AUTO
- all RIDs / timings / retries
- Weapons / Debug overlay behavior

TEST
----
Please test F1 in this order:
1. Reach OFF: Bond should NOT be able to initiate punching.
2. F1 -> LICENSE TO PUNCH: punching should work, lethal firearm use should remain blocked.
3. F1 -> LICENSE TO KILL: lethal firearm use should work.
4. F1 -> OFF: both punch permission and lethal force should be blocked again.

Send QProtocol.log after the test.

FILES
-----
QProtocol.asi
QProtocol.ini
README.txt
