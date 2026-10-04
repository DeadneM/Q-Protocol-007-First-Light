Q Protocol - Fresh Core A13
===========================

OCTOBER FIREARM RID REMAP TEST

Canonical gameplay base
-----------------------
Fresh Core A5 remains the validated gameplay core.

A13 purpose
-----------
A13 continues directly from A12/A11 without changing any gameplay primitive.

The A11 runtime sweep was cross-checked against Glacier Bond-Hashes. This resolves
the current October TEMP resource paths for firearm graphs that were using stale
pre-update IDs.

Remapped firearm aliases
------------------------
AgencyFocusGun          -> 0198799A3CC4E437
AssassinHandcannon      -> 017D8BD5B237B333
ShotgunStandard         -> 01BD00B144B74AC0
AssaultRiflePirate      -> 0110285AF7A95A01
SocomPistol             -> 01D73DA578C4F423
LightPistolLargeMag     -> 010ABE3F66032326
AssaultRifleNonLethal   -> 016D12D89A0A658E
SMGNonLethal            -> 018C90273080F785
BurstPistol             -> 015E2DA84660F7D9
ServicePistol           -> 01988D661ADF3BCE

New current firearm:
ShotgunCompact          -> 01833561121578C4

Validation policy
-----------------
Remapped/new entries start as Experimental, not falsely Validated.
A13 shows Experimental entries in Manual/Automatic selectors by default so they
can be tested immediately.

Previous stale IDs are preserved under [WeaponPreviousRid].

Runtime Discovery cleanup
-------------------------
Brick 0104F2D1C752B7A4 and Vase 01A05C4FEBD7B301 are classified as Throwable.

Only these two A11 RIDs remain without a Bond-Hashes name:
0142DF24DDF6819F
018DCB210A8B048B

Manual / Automatic
------------------
Both selectors read the corrected WeaponCatalog, so ManualLoadout, AutoLoadout
and weapon hotkeys use the new RIDs through the SAME A5 GiveWeapon queue.

The user's other configuration is preserved, including Auto.Enabled=0.

Test
----
1. Open Loadout and confirm remapped Experimental entries are selectable.
2. Test remapped OneHanded and TwoHanded entries one by one.
3. Mark successful entries Validated in Weapons.
4. Confirm F9/F10 now use the corrected SMGNonLethal / AssaultRifleNonLethal IDs.

A13 is a TEST candidate and is not canonical until gameplay validation.
