Q Protocol - Fresh Core A6
==========================

TEST BUILD

Validated base
--------------
Fresh Core A5 is VALIDATED and remains the canonical gameplay base.

A6 goal
-------
Add the first real Insert configuration overlay without redesigning the
validated gameplay core.

Overlay architecture
--------------------
- Native Win32 overlay window created by the same ASI.
- Renderer-independent: no DX11/DX12 Present hook is added.
- Insert opens/closes the overlay.
- The overlay writes the SAME QProtocol.ini used by the gameplay core.
- Save / Reload / Reset Defaults are implemented.
- While the overlay is open, manual F1-F12 actions are suppressed.
- Weapon queue and native gameplay processing continue normally.

A6 editable settings
--------------------
Status:
- Player READY / NOT READY
- AUTO WAITING / DONE
- weapon queue count
- next Q-Pistol mode

Profiles:
- AUTO Enabled
- F2 Profile = Manual / Auto
- F3 Profile = Manual / Auto
- ManualLoadout QPistol / OneHanded / TwoHanded
- ManualAmmo six confirmed firearm classes
- AutoLoadout QPistol / OneHanded / TwoHanded
- AutoAmmo six confirmed firearm classes

Weapon selectors are populated from [WeaponCatalog] in QProtocol.ini.

Profile= is now functional
--------------------------
[Hotkey_F2]
Profile=Manual -> F2 uses [ManualAmmo]
Profile=Auto   -> F2 uses [AutoAmmo]

[Hotkey_F3]
Profile=Manual -> F3 uses [ManualLoadout]
Profile=Auto   -> F3 uses [AutoLoadout]

Preserved from A5
-----------------
- F1 License To Kill
- native F2 AddAmmo primitive
- typed F3 three-role loadout
- F4 Q-Pistol swap
- F5-F12 individual weapons
- shared GiveWeapon queue
- 500 ms inter-weapon stabilization
- AUTO using the same GiveWeapon/AddAmmo primitives

Test
----
1. Enter a playable mission and confirm A5 gameplay behavior is unchanged.
2. Press Insert.
3. Confirm the overlay appears and is clickable.
4. Change one Manual/Auto weapon or ammo value.
5. Press Save.
6. Close with Insert and confirm the corresponding F2/F3 behavior uses the saved values.
7. Change F2 or F3 Profile between Manual and Auto and verify the selected profile is used.
8. Test Reload and Reset Defaults.
9. Confirm F1-F12 do not trigger while the overlay is open.
10. Confirm AUTO still runs once per READY cycle and no crash occurs on respawn/level change.

If the overlay does not appear, attach QProtocol.log.
