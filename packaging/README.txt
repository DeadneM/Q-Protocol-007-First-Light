Q Protocol - Fresh Core A20I Native LTK State Toggle TEST
=============================================================

BASE
----
A20H:
- Weapons tab = configurable F5-F12 slots
- Debug tab = old catalogue / Spawn Weapon / Runtime Discovery
- A20F direct native weapon graphs remain unchanged

A20I CHANGE
-----------
F1 now reads the actual License To Kill state calculated by the game at runtime,
then forces the opposite state.

At the moment F1 is pressed:
- native LTK OFF -> FORCE ON
- native LTK ON  -> FORCE OFF

This also works on the first F1 press after launch once the game has evaluated
its LTK state at least once.

IMPLEMENTATION
--------------
A tiny observation hook records the game's DL boolean immediately after the
native Rules of Engagement / LTK calculation. It does not alter the calculation.

F1 uses that observed value and then controls both native result paths:
- FORCE ON makes both paths resolve true
- FORCE OFF makes both paths resolve false

DLL shutdown restores:
- both original LTK instructions
- the original observation-hook bytes

TEST
----
1. Start a mission where License To Kill is naturally OFF.
2. Press F1 once: lethal force should become allowed.
3. Press F1 again: lethal force should become disallowed.
4. Start / enter an area where License To Kill is naturally ON.
5. Before using F1 in that area, press F1 once: lethal force should become OFF.
6. Send QProtocol.log.

FILES
-----
QProtocol.asi
QProtocol.ini
README.txt
