Q Protocol v0.9.0 - Fresh Core A16
========================================

PUBLIC RELEASE BUILD

Gameplay foundation
-------------------
Fresh Core A5 remains the validated gameplay primitive base.

A16 release change
------------------
Experimental weapons are HIDDEN from Manual and Automatic loadout lists by
default.

The overlay still includes:
  Show Experimental weapons in loadout lists

Enable that checkbox only when you want to test Experimental catalogue entries.

Validated weapons remain available normally.

Weapon catalogue
----------------
The A15 audited catalogue is preserved unchanged:
- no duplicate WeaponCatalog RIDs;
- no WeaponCatalog / RuntimeCatalog overlap;
- no current weapon reuses an archived stale RID;
- all weapon aliases have a role and validation status.

The established compact shotgun aliases remain:
- ShotgunCompact          01833561121578C4  TwoHanded
- ShotgunCompactOneHanded 018DCB210A8B048B  OneHanded
- AssassinRifle           0142DF24DDF6819F  TwoHanded

Gameplay architecture
---------------------
No gameplay primitive changed for this release.

Manual and AUTO still share:
- ResolvePlayer()
- GiveWeapon()
- AddAmmo()
- the same 500 ms weapon stabilization
- the same native gameplay hook

Installation
------------
Copy these files into the game directory where Q Protocol is installed:
- QProtocol.asi
- QProtocol.ini
- README.txt

Overlay key: Insert

Default controls
----------------
F1  License To Kill
F2  Ammo
F3  Manual loadout
F4  Q-Pistol swap
F5-F12 configurable weapon hotkeys
