Q Protocol - Fresh Core A14
===========================

COMPLETED A11 FIREARM CATALOG TEST

Canonical gameplay base
-----------------------
Fresh Core A5 remains the validated gameplay core.

A14 purpose
-----------
A14 completes the A11 runtime firearm catalogue.

New identifications supplied from in-game testing:
- ShotgunCompactTwoHanded  = 01833561121578C4
- ShotgunCompactOneHanded  = 018DCB210A8B048B
- AssassinRifle            = 0142DF24DDF6819F

Roles
-----
ShotgunCompactTwoHanded = TwoHanded
ShotgunCompactOneHanded = OneHanded
AssassinRifle           = TwoHanded

Validation policy
-----------------
These three entries are added as Experimental so they appear in Manual/Auto
selectors immediately without being falsely promoted to Validated.

A14 therefore accounts for every RID observed in the A11 41-graph runtime sweep.

Manual / Automatic
------------------
Both selectors use the corrected WeaponCatalog directly.

No gameplay primitive changed. Manual, Auto and hotkey requests still use the
same A5 GiveWeapon queue and 500 ms stabilization.

Test
----
1. Open Loadout.
2. Confirm both Shotgun Compact variants appear in the correct role lists.
3. Confirm AssassinRifle appears under TwoHanded.
4. Test each one in Manual and Auto.
5. Promote successful entries to Validated from the Weapons tab.

A14 is a TEST candidate until in-game validation.
