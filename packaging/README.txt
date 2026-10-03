Q Protocol - Fresh Core A10
===========================

WEAPON REVALIDATION + UI FIX TEST

Canonical gameplay base
-----------------------
Fresh Core A5 remains the validated gameplay core.

A10 purpose
-----------
A9's overlay/render path is retained.

A10 fixes two user-reported issues:

1. weapon validation metadata was carrying legacy pre-October status;
2. ammo controls were too cramped and the ImGui +/- step buttons could be cut.

Weapon catalogue policy
-----------------------
The alias/RID catalogue originally came from the pre-October Q Protocol branch.

The Fresh Core rebuilt the gameplay primitives for the October executable, but
the full weapon catalogue itself was not rediscovered from scratch.

Therefore A10 resets Status conservatively:

Validated
- only weapons directly exercised successfully on the October Fresh Core

Not Working
- only weapons directly observed failing on the October Fresh Core

Experimental
- known/legacy weapon alias + RID that still needs re-validation on October

A10 October-Validated defaults:
- QPistolSilenced
- QPistolUnsilenced
- HeavyPistol50Cal
- ARMilitary
- Taser
- MachinePistolHighRecoil
- ShotgunSemiAuto
- LightPistolNonLethal
- AssaultRifleNonLethal
- SMGNonLethal

A10 known Not Working:
- AgencyFocusGun
- SocomPistol
- BurstPistol

All other catalogue entries start as Experimental.

The user can re-test each weapon in Weapons -> Spawn Weapon and manually change
its status to Validated / Not Working / Experimental, then Save.

Important:
A10 does not claim that an Experimental RID is wrong. It only means that the
current October build has not yet re-confirmed it.

Ammo UI fix
-----------
Manual and Automatic ammo are now displayed as one clean row per ammo class.

Each row has:
- ammo class label
- one wider numeric input field

The tiny ImGui +/- steppers were removed entirely. This avoids clipping and is
simpler to use.

The main overlay was also enlarged slightly to 980 x 740 maximum.

Spawn Weapon
------------
Unchanged from A9:

Weapons -> Spawn Weapon -> atomic RID mailbox -> A5 WorkerThread -> QueueWeapon()

No native spawn call occurs from the ImGui render thread.

Test
----
1. Confirm the overlay still opens with Insert.
2. Confirm ammo fields are fully visible and no +/- controls are clipped.
3. Open Weapons and review the conservative October statuses.
4. Test Experimental weapons with Spawn Weapon.
5. Change each tested weapon to Validated or Not Working and press Save.
6. Reopen the overlay and confirm statuses persist.
7. Confirm A5 gameplay/F1-F12/AUTO remain unchanged.

This build is intended to rebuild the October weapon truth table progressively.
