Q Protocol - Fresh Core A20B
===========================

RESOLVER DIAGNOSTIC + COSMETIC DISCOVERY TEST

Purpose
-------
A20 is the research build for the requested NG+-style cosmetic unlock feature:

- Unlock All Outfits
- Unlock All Weapon Skins
- Unlock All Gadget Skins
- future master toggle: Unlock All Cosmetics

A20 DOES NOT unlock anything yet.

It installs read-only runtime probes on the current October 2026 executable and
logs the structures used by:

- $knt.loadout.firearmSkins
- $knt.loadout.gadgetSkins
- $knt.outfits.outfits
- $knt.online.unlockables

No save writes
--------------
A20 does not edit data.save or index.save.
A20 does not call knt.online.unlockable.acquire.
A20 does not alter challenge completion or unlock states.

Test procedure
--------------
1. Install A20 over Q Protocol.
2. Start the game normally.
3. Open Customisation / TacSim.
4. Browse:
   - Outfits
   - Weapon skins
   - Gadget skins
5. Spend a few seconds in each category.
6. Exit the game.
7. Send QProtocol.log back for analysis.

Expected log prefix:
[COSDISC]

Configuration
-------------
[CosmeticDiscovery]
Enabled=1
MaxCallsPerHook=8

Set Enabled=0 to disable the A20 probes.

All A19 gameplay functionality is preserved.
Fresh Core A5 remains the validated gameplay primitive base.


A20B resolver diagnostic
------------------------
A20B adds no new gameplay writes. It records the exact reason the shared player
resolver becomes unavailable.

Look for:
RESOLVE FAIL stage=...
RESOLVE RECOVERED ...

This is specifically intended to diagnose Manual/AUTO weapon availability in
TacSim/customisation and after returning to gameplay.
