Q Protocol - Fresh Core A5
==========================

TEST BUILD

Validated base
--------------
Fresh Core A4 is treated as VALIDATED from the user's positive test feedback.
A4 preserves the A3D weapon core and adds the native F2 reserve-ammo primitive.

A5 addition: minimal AUTO
-------------------------
AUTO is deliberately not a second weapon/ammo engine.

When a new playable READY cycle is detected and [Auto] Enabled=1, A5 does exactly:

  QueueLoadout(AutoLoadout)
  QueueAmmoProfile(AutoAmmo)
  AutoDone = true

The same shared primitives are used by manual controls:
- F2 and AUTO share the same native AddAmmo path.
- F3 and AUTO share the same typed three-role GiveWeapon queue.
- The shared 500 ms inter-weapon stabilization remains unchanged.

AUTO profile
------------
[AutoLoadout]
QPistol   = one Q-Pistol variant
OneHanded = one one-handed firearm
TwoHanded = one two-handed firearm

[AutoAmmo]
QPistol
SMG
AssaultRifle
Shotgun
Sniper
HeavyPistol

Every selection/quantity is configurable independently from ManualLoadout/ManualAmmo.

Re-arm behavior
---------------
AUTO re-arms only when the shared player context leaves READY or its player identity changes.

A loadout pointer refresh while the same player remains READY does NOT re-arm AUTO.
This is intentional because GiveWeapon itself can refresh the loadout pointer; re-arming there
would create an automatic loop.

Preserved
---------
- F1 License To Kill
- F2 native ManualAmmo
- F3 typed ManualLoadout
- F4 Q-Pistol swap
- F5-F12 configured weapons
- shared GiveWeapon queue
- 500 ms inter-weapon stabilization
- native AddFirearmAmmunitionToPlayer event 0x1DE

Still inactive
--------------
- overlay

Test
----
1. Keep [Auto] Enabled=1.
2. Enter a playable mission without pressing F2/F3.
3. Confirm AUTO gives exactly the three configured AutoLoadout roles.
4. Confirm AutoAmmo reserve quantities are added.
5. Confirm AUTO runs only once while remaining in the same playable context.
6. Press F2 and F3 manually afterward and confirm both still work.
7. Change level / respawn and confirm AUTO re-arms on the next READY cycle.
8. Confirm F1/F4/F5-F12 remain functional and there is no crash.

Expected log:
AUTO committed once for current READY cycle: weapons=3 ammo=queued.
AUTO Ammo AddAmmo class=...
AUTO Ammo PUBLISHED through native ammo event 0x1DE
