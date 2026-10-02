Q Protocol - Fresh Core A3B
===========================

TEST BUILD

Validated base
--------------
Fresh Core A2 remains the canonical base.

A3 was rejected before validation because it modeled ManualLoadout as an arbitrary list of up to 8 weapons.

Correct model in A3B
--------------------
The game supports exactly three loadout weapon roles:

1. QPistol
2. OneHanded
3. TwoHanded

QProtocol.ini now uses:

[ManualLoadout]
QPistol=QPistolSilenced
OneHanded=MachinePistolHighRecoil
TwoHanded=ShotgunSemiAuto

F3 queues exactly those three typed roles through the SAME GiveWeapon() queue validated in A2.

F5-F12 remain independent standalone weapon hotkeys. They are not loadout slots.

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
4. Confirm:
   - one Q-Pistol variant is present;
   - one one-handed weapon is present;
   - one two-handed weapon is present.
5. No fourth loadout weapon should be introduced.

Expected log:
- ManualLoadout QPistol = ...
- ManualLoadout OneHanded = ...
- ManualLoadout TwoHanded = ...
- F3 ManualLoadout QPistol queued RID=...
- F3 ManualLoadout OneHanded queued RID=...
- F3 ManualLoadout TwoHanded queued RID=...
- F3 ManualLoadout queued 3/3 role(s).
