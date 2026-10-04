Q Protocol - Fresh Core A12
===========================

RUNTIME ITEM CATALOG TEST

Canonical gameplay base
-----------------------
Fresh Core A5 remains the validated gameplay core.

A12 purpose
-----------
A12 is a surgical continuation of A11.

It keeps:
- the A5 gameplay primitives unchanged;
- A11 runtime ItemEntry/Spawner discovery;
- Q-Pistol Off;
- the A9/A10/A11 DX12 overlay;
- the user's complete Validated / Not Working / Experimental weapon status table.

It adds only a runtime item catalogue so known non-firearm graphs stop appearing
as anonymous weapon candidates.

Identified A11 runtime items
----------------------------
MissilePen       01400A15903C9985   Gadget
Laser            01453F3961FC0BB7   Gadget
BlastDevice      015314707AE716BF   Gadget
ShockWave        011B83C48DAC20CA   Gadget
SmokePellets     01BA24E28342EA32   Gadget
Hack             019CF34A2C59C76F   Gadget
Dartgun          01C315FC8C1AEF95   Gadget
GrenadeFlashNPC  017D301CA6D6BF4E   Grenade

Runtime Discovery
-----------------
Weapons -> Runtime Discovery now shows:
- resolved item name when known;
- category (Gadget / Grenade / Unknown);
- current-level presence;
- Identified vs Uncatalogued state;
- exact RID;
- the existing Spawn Weapon test button.

Unknown RIDs remain visible. A12 deliberately does not invent names for them.

Important
---------
RuntimeCatalog is separate from WeaponCatalog.

This prevents gadgets and throwables from appearing in Manual/Automatic firearm
loadout lists while still preserving them as useful runtime-discovery evidence.

Configuration
-------------
This test package preserves the user's supplied A11 INI state, including:
- Auto.Enabled=0;
- all current hotkey/loadout choices;
- all current Validated / Not Working / Experimental classifications.

Gameplay architecture
---------------------
No gameplay primitive changed.

Catalog and Runtime Discovery still use:

overlay -> display RID -> A5 WorkerThread -> RotateRid -> QueueWeapon()

Test
----
1. Open Weapons -> Runtime Discovery.
2. Confirm the seven gadgets and Grenade Flash NPC show names instead of bare RIDs.
3. Confirm the remaining unidentified RIDs still show as Uncatalogued.
4. Confirm Weapons -> Catalog keeps all existing weapon statuses.
5. Confirm Manual/Automatic loadout lists contain firearms only.
6. Test Spawn Weapon on unknown RIDs as before.

A12 is a TEST candidate and is not canonical until validated in game.
