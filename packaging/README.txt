Q Protocol - Fresh Core A17
===========================

OPTIONAL ONE/TWO-HANDED LOADOUT TEST

Gameplay foundation
-------------------
Fresh Core A5 remains the validated gameplay primitive base.

A17 changes
-----------
1. OneHanded and TwoHanded selectors now support Off / None in both Manual and
   Automatic profiles.
2. Public Automatic defaults are now:
     QPistol   = QPistolSilenced
     OneHanded = Off
     TwoHanded = Off
3. Reset Defaults now matches the shipped public defaults:
     Auto.Enabled = 0
     Auto OneHanded = Off
     Auto TwoHanded = Off
4. User-confirmed weapon status updates from the supplied October INI are merged.
5. Experimental weapons remain hidden from loadout selectors by default.

User-confirmed promotions to Validated
---------------------------------------
AssaultRiflePirate
SocomPistol
LightPistolLargeMag
ShotgunCompact
ShotgunCompactOneHanded
AssaultRifleNonLethal
SMGNonLethal
ServicePistol

Off behavior
------------
Off is stored as None in QProtocol.ini.

The existing shared core already treats None as RID 0, so no new gameplay writer,
spawn path or AUTO implementation is introduced.

No gameplay primitive changed
-----------------------------
ResolvePlayer, GiveWeapon, AddAmmo, the native gameplay hook and the 500 ms
weapon stabilization are unchanged.

A17 is a TEST candidate until in-game validation.
