Q Protocol - Fresh Core A9
==========================

ARSENAL OVERLAY TEST

Canonical gameplay base
-----------------------
Fresh Core A5 remains the validated gameplay core.

A9 overlay architecture
-----------------------
A9 removes the A8 late DXGI-factory dependency.

Instead it:
1. creates temporary local D3D12/DXGI probe objects;
2. reads the runtime addresses of:
   - IDXGISwapChain::Present
   - IDXGISwapChain::ResizeBuffers
   - ID3D12CommandQueue::ExecuteCommandLists
3. hooks those runtime functions globally with MinHook;
4. captures the game's first real DIRECT command queue;
5. identifies the real game swapchain by process/window/device;
6. initializes Dear ImGui only on that real swapchain.

This works even if the game's swapchain existed before QProtocol.asi loaded.

A5 gameplay remains fail-open:
if the overlay does not initialize, F1-F12 and AUTO remain active.

Overlay tabs
------------
Loadout
- Manual Q-Pistol / One-handed / Two-handed
- Automatic Q-Pistol / One-handed / Two-handed
- Manual and Automatic reserve ammo
- Automatic enable switch
- Experimental weapons can optionally be shown in loadout selectors
- Not Working weapons remain hidden from loadout selectors

Weapons
- full known firearm list
- search
- Role
- TEMP RID
- Status

Weapon Status
-------------
Each weapon has one editable status:

Validated
Not Working
Experimental

Status is stored in [WeaponValidation] in QProtocol.ini.

Current known Not Working entries from previous tests:
- AgencyFocusGun
- SocomPistol
- BurstPistol

The user can change any status from the Weapons tab and press Save.

Spawn Weapon
------------
The Weapons tab has exactly one gameplay action button:

Spawn Weapon

The overlay does NOT call native Spawn itself.

It places the selected display RID in an atomic mailbox.
The A5 WorkerThread consumes that request and calls the existing:

QueueWeapon() -> GiveWeapon

Therefore Spawn Weapon uses the same validated queue, pair-clone path and
500 ms stabilization as F4/F5-F12.

Hotkeys
-------
The Hotkeys tab currently shows the authoritative mapping.
Hotkey rebinding is intentionally not part of A9.

Controls
--------
Insert   = open/close overlay
Save     = save loadouts/ammo/statuses and reload runtime configuration
Reload   = reload QProtocol.ini
Defaults = restore Manual/Automatic profile defaults

Test
----
1. Confirm A5 hotkeys and AUTO still work before opening the overlay.
2. Press Insert.
3. Confirm the in-game ImGui overlay appears.
4. Open Weapons.
5. Select a Validated weapon and press Spawn Weapon.
6. Confirm the weapon is queued/given using the normal A5 path.
7. Change a weapon Status, press Save, close/reopen and confirm it persists.
8. Test an Experimental/Not Working weapon if desired.
9. Confirm no crash on Alt-Tab, resize, respawn or mission transition.

If the overlay does not appear, attach QProtocol.log.
