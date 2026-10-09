Q Protocol - Fresh Core A20N Live Input Transition Trace TEST
=============================================================

BASE
----
Validated public v0.9.4 / Fresh Core A20J Clean Core
plus the A20K three-state Rules of Engagement cycle.

WHY A20N
--------
A20M proved that scanning vtable instances after PLAYER READY was observing
persistent definitions/templates rather than the live close-combat blocker.

Across multiple F1 states and a player transition, the scanned manager,
InputConfig, BlockPlayerInputAction candidates and Block/Unblock marker objects
remained unchanged.

A20N therefore stops broad memory scanning and traces the actual state-transition
function at runtime.

LIVE TRACE
----------
A20N hooks the October executable function at:

EXE+0x016D0830

Static audit shows:
- RCX = ZCLBlockPlayerInputAction*
- DL  = requested state (0 = UNBLOCK, nonzero = BLOCK)

The hook is diagnostic only:
- it does NOT change DL
- it does NOT force any close-combat permission
- it queues the object pointer, requested state, caller address and pre-transition
  object state
- the original 16-byte prologue is replayed exactly
- execution resumes at EXE+0x016D0840

LOG FORMAT
----------
Look for:

A20N INPUT TRANSITION

Each line includes:
- caller RVA when the caller belongs to 007FirstLight.exe
- object pointer
- desired BLOCK / UNBLOCK
- pre-transition +0x60 / +0x61 / +0x62 state bytes
- Input / Target / Runtime first qwords

This should expose which live gameplay path blocks close combat in TacSim.

F1
--
The A20K cycle remains unchanged:

OFF -> LICENSE TO PUNCH -> LICENSE TO KILL -> OFF

TEST PLAN
---------
Best test:

1. Start a normal mission where Bond can punch.
2. Wait until fully playable.
3. Press F1 through the complete OFF / PUNCH / LTK cycle.
4. Leave or change level.
5. Enter TacSim, where punching is unavailable.
6. Repeat the F1 cycle.
7. Send QProtocol.log.

A20N starts tracing before PLAYER READY so transient block/unblock calls during
loading are captured too.

UNCHANGED
---------
- A20J weapon architecture
- F2 ammo
- F3 Manual loadout
- F4 Q-Pistol swap
- F5-F12 weapon slots
- AUTO
- all weapon RIDs / timings / retries
- overlay behavior and INI defaults

FILES
-----
QProtocol.asi
QProtocol.ini
README.txt
