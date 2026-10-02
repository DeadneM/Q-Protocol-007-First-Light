Q Protocol - Fresh Core A4
==========================

TEST BUILD

Validated base
--------------
Fresh Core A3D is now VALIDATED and is the canonical base.

A3D validation
--------------
The user confirmed the typed F3 loadout works correctly with the shared
500 ms inter-weapon stabilization delay.

Preserved without redesign:
- F1 License To Kill
- F3 typed ManualLoadout: QPistol + OneHanded + TwoHanded
- F4 Q-Pistol swap
- F5-F12 configured weapons
- shared GiveWeapon queue and 500 ms stabilization delay

A4 addition: F2 native reserve ammo
------------------------------------
F2 now calls the current October-2026 gameplay primitive:

  Gameplay::SGpwInput_AddFirearmAmmunitionToPlayer

This deliberately DOES NOT use the lower-level 24-byte SetFirearmAmmo path
that caused the rejected U80-era rebase experiments.

Current native AddAmmo path:
- ammo owner global: EXE+0x064576E0
- lock/context:       owner+0x238E0
- input vector:       owner+0x20B70
- input pool/context: owner+0x20AD0
- vector insert:      EXE+0x00116170
- publish helper:     EXE+0x012A8FC0
- native event:       0x1DE

Native input record remains exactly 12 bytes:
  uint32 playerId
  uint32 amount
  uint32 firearmClass

Confirmed classes
-----------------
0 = Q-Pistol
1 = SMG / MachinePistol
2 = Assault Rifle
5 = Shotgun
6 = Sniper / Marksman
7 = Heavy Pistol .50

Classes 3 and 4 remain disabled/unpublished.

ManualAmmo values are additive reserve quantities. They do not replace the
magazine and they do not set a target total.

Still inactive
--------------
- AUTO
- overlay

Test
----
1. Reach a playable mission.
2. Use some ammunition from the equipped weapons.
3. Press F2 once.
4. Confirm reserve ammo increases according to [ManualAmmo].
5. Press F2 again and confirm the refill is additive/repeatable.
6. Confirm F1, F3, F4 and F5-F12 still work.
7. Confirm no crash during level change/respawn.

Expected log:
F2 ManualAmmo queued for playerId=...
F2 AddAmmo class=... amount=+...
F2 ManualAmmo PUBLISHED through native event 0x1DE
