Q Protocol - Fresh Core A3C
===========================

TEST BUILD

Validated base
--------------
Fresh Core A2 remains canonical.

A3B result
----------
F3 gave only the Q-Pistol.

Root cause in the fresh-core logic:
- F3 queued all three typed roles correctly;
- after the first GiveWeapon(), the game refreshed the loadout pointer;
- A3B treated any loadout pointer change as a new player generation;
- ResetWeaponRuntime() therefore cleared the remaining OneHanded and TwoHanded requests.

A3C correction
--------------
A full weapon-runtime reset now happens only when:
- the player becomes unavailable; or
- playerId changes.

If the loadout pointer changes while playerId stays the same:
- the queued weapons are preserved;
- the graph cache is invalidated;
- the next queued role resolves against the refreshed game state.

Typed loadout remains exactly:
1. QPistol
2. OneHanded
3. TwoHanded

F5-F12 remain independent weapon hotkeys.

Still inactive
--------------
- F2 ammo
- AUTO
- overlay

Test
----
1. Reach a playable mission.
2. Press F3 once.
3. Confirm all three roles are present:
   - configured Q-Pistol variant
   - configured one-handed weapon
   - configured two-handed weapon
4. Confirm F1/F4/F5-F12 remain working.

Expected additional log line:
PLAYER LOADOUT REFRESH ... - preserving weapon queue.
