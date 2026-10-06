Q Protocol - Fresh Core A19R Runtime AUTO Cycle TEST
======================================================

BASE
----
Strictly based on public v0.9.1 / Fresh Core A19 Audit Hardening.

PURPOSE
-------
Restore the pre-Fresh-Core AUTO trigger model without restoring the old U74
state-machine complexity.

ONLY BEHAVIORAL CHANGE
----------------------
- Weapon/loadout readiness and runtime-player readiness are observed separately.
- Manual weapon actions use the validated weapon/loadout context.
- F2 / AUTO ammo still require a real runtime playerId.
- AUTO is armed/re-armed ONLY by the runtime-player lifecycle:
  * runtime player becomes READY
  * runtime player leaves READY
  * runtime playerId changes
- INI Reload does NOT re-arm AUTO.
- Loadout pointer refresh does NOT re-arm AUTO.
- Once armed, AUTO uses the same existing QueueLoadout / GiveWeapon / AddAmmo
  primitives already validated in A19.

NOT CHANGED
-----------
GiveWeapon
AddAmmo
Weapon RIDs
Ammo classes
500 ms weapon queue delay
Gameplay hook
Overlay renderer
Manual/Auto profile format

IMPORTANT TEST PACKAGING
------------------------
This ZIP intentionally contains NO QProtocol.ini.
Keep your existing v0.9.1 QProtocol.ini so your AUTO weapon choices are not
overwritten by test defaults.

TEST
----
1. Keep your existing QProtocol.ini.
2. Replace only QProtocol.asi with this test ASI.
3. Enable AUTO with obvious OneHanded and TwoHanded weapons.
4. Start normal gameplay and confirm AUTO once.
5. Enter TacSim and confirm AUTO re-applies there without pressing F3.
6. Leave TacSim and confirm AUTO re-arms on the next runtime-player cycle.
7. Send QProtocol.log.

Public release remains v0.9.1 / Fresh Core A19.
