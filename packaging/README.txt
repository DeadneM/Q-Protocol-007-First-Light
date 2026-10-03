Q Protocol - Fresh Core A11
===========================

RUNTIME WEAPON DISCOVERY TEST

Canonical gameplay base
-----------------------
Fresh Core A5 remains the validated gameplay core.

A11 purpose
-----------
A11 keeps the working A10/A9 DX12 Arsenal overlay and adds two targeted features:

1. Q-Pistol Off for Manual and Automatic loadouts;
2. runtime RID discovery to recover changed/missing weapon IDs.

Q-Pistol Off
------------
ManualLoadout.QPistol and AutoLoadout.QPistol now expose:

- Off
- QPistolSilenced
- QPistolUnsilenced

The overlay stores Off as:

QPistol=None

This is already supported by the A5 parser and simply produces RID 0.
No new Q-Pistol gameplay path is introduced.

Weapon status preservation
--------------------------
The user's current [WeaponValidation] table is authoritative and is preserved.

A11 never deletes Not Working or Experimental entries.

Those entries are intentionally retained because they are the candidates whose
current October IDs may need to be rediscovered.

Runtime Discovery
-----------------
The Fresh Core already builds a runtime map of loaded
ItemEntry/Spawner RID graphs.

A11 publishes those graph RIDs to the overlay.

Weapons now contains two subtabs:

Catalog
- existing named weapon catalogue;
- editable Validated / Not Working / Experimental status;
- Spawn Weapon.

Runtime Discovery
- RIDs present in the runtime graph set but absent from WeaponCatalog;
- whether each RID is present in the current level or was seen earlier in the
  same game session;
- Spawn Weapon for an uncatalogued RID.

The session list is cumulative across level transitions.

This provides a practical method to recover:
- replacement October IDs for Not Working legacy weapons;
- weapons missing entirely from the current catalogue.

Important
---------
A runtime graph RID is not automatically declared a firearm name.

The discovery list deliberately labels it as Uncatalogued until its spawned
result is identified.

The log also records every newly seen runtime graph RID as:

DISCOVERY runtime graph RID=XXXXXXXXXXXXXXXX

This allows multiple levels to be audited from one QProtocol.log.

Spawn architecture
------------------
Both Catalog and Runtime Discovery use the same mailbox:

overlay -> display RID -> A5 WorkerThread -> RotateRid -> QueueWeapon()

No native Spawn call is made from the render thread.

Test
----
1. Open Loadout and confirm Q-Pistol has an Off option in both Manual and Automatic.
2. Save Q-Pistol Off and confirm the INI contains QPistol=None.
3. Confirm Manual/Auto then skip the Q-Pistol role while other roles still work.
4. Open Weapons -> Catalog and confirm your statuses are preserved.
5. Open Weapons -> Runtime Discovery.
6. Note current/runtime unknown RIDs.
7. Spawn uncatalogued RIDs one at a time and identify what appears in game.
8. Visit several levels; the session discovery list should accumulate new RIDs.
9. Send QProtocol.log after the sweep so replacement/missing IDs can be mapped.

A5 gameplay primitives remain unchanged.
