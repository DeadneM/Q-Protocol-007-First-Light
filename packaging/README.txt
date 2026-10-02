Q Protocol - Fresh Core A3
==========================

TEST BUILD

Validated base
--------------
Fresh Core A2 is the current validated canonical base.

Confirmed working in A2:
- ResolvePlayer()
- F1 License To Kill
- shared GiveWeapon()
- F4 Q-Pistol swap
- F5-F12 configured weapon slots
- minimal gameplay-thread native spawn hook

New in A3
---------
F3 now applies [ManualLoadout].

Implementation is deliberately thin:
- read Weapon1..Weapon8 from [ManualLoadout]
- skip None / empty slots
- queue every configured weapon into the SAME GiveWeapon() queue validated in A2

There is no separate F3 weapon engine and no new gameplay hook.

Still intentionally inactive
----------------------------
- F2 ammo
- AUTO
- overlay

Test
----
1. Reach a playable mission.
2. Verify F1/F4/F5-F12 still work.
3. Press F3 once.
4. Confirm the configured ManualLoadout weapons are given one after another.
5. Send QProtocol.log if anything differs from expected.

Expected F3 log lines:
- ManualLoadout Weapon1 = ...
- ManualLoadout Weapon2 = ...
- ...
- F3 ManualLoadout Weapon1 queued RID=...
- GiveWeapon COMPLETE RID=...
- F3 ManualLoadout queued N weapon(s).

A3 must not change the already validated A2 GiveWeapon behavior.
