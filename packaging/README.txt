Q Protocol - Fresh Core A19T2 TacSim Auto Rearm TEST
======================================================

BASE
----
Strictly built from the validated public v0.9.1 / Fresh Core A19 Audit Hardening,
through the isolated A19T TacSim readiness candidate.

A19T2 CHANGE
-------------
A19T proved Manual weapon operations work with the split WeaponReady/AmmoReady
model. The log also proved AUTO GiveWeapon itself succeeds when triggered.

The remaining bug was the AUTO one-shot latch:
- editing/reloading AutoLoadout did not reset autoWeaponsDone;
- a live loadout pointer refresh did not reset autoWeaponsDone.

A19T2 changes only those two re-arm points.

On INI Reload:
- autoWeaponsDone = false
- autoAmmoDone = false

On weapon loadout pointer refresh:
- autoWeaponsDone = false

No weapon RID, GiveWeapon implementation, native spawner, graph scan, ammo native
path, queue timing or overlay renderer is changed.

TEST
----
1. Enter TacSim.
2. Configure AUTO with obvious OneHanded + TwoHanded choices.
3. Save/Reload from the overlay.
4. Confirm AUTO applies immediately without pressing F3.
5. Change the AUTO weapons again and Reload; confirm the new profile applies.
6. Leave/re-enter TacSim and confirm AUTO applies once to the refreshed loadout.
7. Send QProtocol.log.

Public release remains v0.9.1 / A19 until validated.
