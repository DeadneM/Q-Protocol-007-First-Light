Q Protocol - Fresh Core A1
==========================

TEST BUILD

Purpose
-------
This is the first fresh-source Q Protocol build after the October 2026 game update.

Implemented:
- clean ASI bootstrap
- QProtocol.log reset on every launch
- October executable validation
- shared ResolvePlayer()
- F1 License To Kill ON/OFF

Intentionally NOT implemented yet:
- F2 ammo
- F3 manual weapon loadout
- F4 Q-Pistol swap
- F5-F12 weapon slots
- AUTO
- overlay

Test
----
1. Copy QProtocol.asi and QProtocol.ini to the game directory using the same ASI loader setup as before.
2. Launch the game.
3. Reach a playable mission.
4. Press F1 twice and verify License To Kill toggles ON then OFF.
5. Send QProtocol.log.

Expected useful log lines:
- Target executable accepted.
- PLAYER READY ...
- F1 License To Kill = ON
- F1 License To Kill = OFF

This A1 build is deliberately small. It is not based on rejected U80-U85 binaries.
