Q Protocol - Fresh Core A15
===========================

AUDITED WEAPON CATALOG TEST

Canonical gameplay base
-----------------------
Fresh Core A5 remains the validated gameplay core.

Why A15 exists
--------------
A full A13/A14 audit found no gameplay-code regression, but it found one
catalogue compatibility mistake in A14:

A13 already used:
ShotgunCompact = 01833561121578C4

A14 unnecessarily renamed that existing alias to ShotgunCompactTwoHanded.
The RID was not duplicated, but renaming the alias could break an existing INI
that already selected ShotgunCompact.

A15 fixes this by preserving the established alias:

ShotgunCompact          = 01833561121578C4   TwoHanded
ShotgunCompactOneHanded = 018DCB210A8B048B   OneHanded
AssassinRifle           = 0142DF24DDF6819F   TwoHanded

No duplicate WeaponCatalog RID is retained.

Audit findings
--------------
- GiveWeapon unchanged.
- AUTO logic unchanged.
- Ammo logic unchanged.
- gameplay hook unchanged.
- 500 ms weapon stabilization unchanged.
- A13 -> A14 overlay code differed only in version/status strings.
- Current WeaponCatalog has unique aliases and unique RIDs.
- WeaponCatalog does not overlap RuntimeCatalog.
- Current weapon RIDs do not reuse archived stale RIDs.

Permanent CI guard
------------------
A15 adds a pre-build catalogue validator. Builds now fail if:
- WeaponCatalog contains duplicate aliases or duplicate RIDs;
- RuntimeCatalog contains duplicate RIDs;
- WeaponCatalog and RuntimeCatalog overlap;
- WeaponRole or WeaponValidation are missing/extra;
- a role/status value is invalid;
- Manual/Auto/F4/F5-F12 reference a missing weapon alias;
- a current weapon accidentally reuses an archived stale RID.

Experimental visibility
-----------------------
Experimental weapons remain visible in Manual/Auto by default. This is an overlay
testing preference only and does not change gameplay primitives.

A15 is a TEST candidate until in-game validation.
