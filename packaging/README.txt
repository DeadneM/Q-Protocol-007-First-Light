Q Protocol - Fresh Core A18
===========================

REMAPPABLE OVERLAY KEY TEST

Gameplay foundation
-------------------
Fresh Core A5 remains the validated gameplay primitive base.

A18 change
----------
The overlay toggle is no longer hardcoded to Insert.

The active key is read from:

[Overlay]
ToggleKey=Insert

The Hotkeys tab now shows an Overlay / Menu toggle key control.

To remap:
1. Open Q Protocol with the current overlay key.
2. Open Hotkeys.
3. Click the current Overlay key button.
4. Press the new keyboard key.
5. Click Save.

Escape cancels key capture.

Supported persistence
---------------------
Common navigation keys, F1-F24, A-Z, 0-9, numpad keys and other captured
Windows virtual keys are saved as readable names or VK_XX fallback values.

Invalid or missing ToggleKey values safely fall back to Insert.

A17 behavior preserved
----------------------
- QPistol / OneHanded / TwoHanded support Off.
- Automatic defaults: QPistolSilenced + Off + Off.
- Experimental weapons hidden from loadout lists by default.
- Latest user-validated weapon statuses preserved.
- Catalogue integrity audit preserved.

No gameplay primitive changed.
A18 is a TEST candidate until in-game validation.
