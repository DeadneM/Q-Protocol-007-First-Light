Q Protocol - Fresh Core A24 Weapon Queue Safety TEST
=====================================================

STATUS
------
TEST candidate. Weapon graph recovery has not yet been validated in-game.
Public stable remains v0.9.4 / A20J.

BASE
----
A24 builds on the A23 dynamic weapon graph recovery and the A22 reliable-input / A21 floating ImGui menu, First Debug DX12 hardening, and the validated v0.9.4 / Fresh Core A20J gameplay core. Experimental F1 branches
A20K-A20N are excluded; close-combat / F1 research is paused.

A24 CRASH INVESTIGATION / QUEUE SAFETY
-------------------------------------
A23 successfully recovers all 41 native weapon graphs, with 42 item entries
and 43 spawners. The supplied A23 log shows 2 AUTO and 2 F3 package
completions, and 41 direct native weapon spawn completions with no logged
gameplay error. However the game terminated immediately after a subsequent
Q-Pistol spawn; this log contains no exception code or crash stack. The
exact cause is NOT established.

The session also includes 33 individual weapon hotkey enqueues and many
repeated F5-F8 requests. A24 guards ONLY F4-F12 and Debug Spawn requests:
- repeated RID requests already active or queued are ignored;
- at most 4 distinct requests can be pending at once;
- at most 12 newly queued individual weapons over 30 seconds;
- recent-request history resets on player READY/NOT READY change;
- requests suppressed by the guard are recorded with "[A24]" in the log;
- A23 41-graph discovery, native spawn invocation, 500 ms cooldown,
  AUTO, F3 Manual Loadout, F1 and F2 gameplay are otherwise unchanged.

A24 is a conservative crash mitigation and diagnostic candidate, not
a proven fix. Further evidence is needed if the game still crashes.

A23 WEAPON GRAPH DISCOVERY REPAIR (RETAINED)
--------------------------------
Observed broken A22 session:
- graph index = 0 ItemEntry/Spawner graphs, 1 candidate;
- AUTO, F3, F4, F5-F12 and Spawn Weapon all fail on graph-not-found;
- F1 native-state toggle and F2 ammo publication appear in the log.

Known working session located the ItemEntry graph at 0x2CE59DC8 while the
player loadout was at 0x2D210E50. In this failing session the player loadout
was at 0x2BDA8038. Q Protocol previously scanned only 0x2C000000-0x30000000.
Changes in allocation layout can put ItemEntry objects below that lower bound.

A23 retains the old scan range and additionally searches around the live
player loadout, 32 MiB in each direction, with no overlapping double scan.
It combines the candidates from both scans and validates the native graph
pointers and weapon RID just as A20J did.

QProtocol.log now reports "A23 graph index:" followed by linked graph count,
ItemEntry RID count, spawner count and scan anchor.

A23 graph discovery is confirmed restored by the user's A23 runtime log.

A22 MOUSE INPUT CORRECTIONS (RETAINED)
--------------------------------------
- The Win32 WndProc no longer skips UI mouse events when the renderer mutex
  is temporarily occupied. A21 used try_to_lock, which could lose clicks.
- While the menu is visible, Win32 mouse events are delivered to ImGui and
  swallowed before they reach the game. A21's WantCaptureMouse gate could be
  stale at the instant of a click.
- WM_INPUT is passed to DefWindowProc for required Windows raw-input cleanup,
  but not to the game's original WndProc while the menu is open.
- Win32 cursor messages handled by ImGui retain the backend's return value.
- First three left-button-down deliveries are logged as
  "A23 ImGui mouse-down received" for local diagnosis.
- When the menu is closed, normal game mouse/raw input handling is restored.
- ImGui keyboard capture remains conditional on WantCaptureKeyboard /
  WantTextInput to preserve A21 keyboard behavior.

A21 FLOATING OVERLAY PRESERVED
------------------------------
- draggable title bar, resize handle and close button;
- one ImGui window, no added native windows or multi-viewports;
- initial size/position only on first use, no frame-by-frame recentering;
- ClipCursor temporarily released while open and restored on close/failure;
- existing Loadout, Weapons, Debug and Hotkeys tabs unchanged.

GAMEPLAY / CONFIG
-----------------
Unchanged from v0.9.4:
- F1 native-state License To Kill toggle
- F2 ammunition
- F3 Manual Loadout
- F4 Q-Pistol swap
- F5-F12 configurable weapon slots
- Automatic loadout / ammo
- native weapon graphs, RIDs, timings and retry behavior
- QProtocol.ini sections, defaults, and saved profile format

First Debug guards remain: precompiled ImGui shaders, checked DX12 resources,
correct tracked swapchain, synchronized Present/ResizeBuffers/fences, fail-open
rendering, diagnostic HRESULT logging and static MSVC runtime.

DEPENDENCIES
------------
ImGui v1.91.5 and MinHook v1.3.4 are compiled into QProtocol.asi.
No separate ImGui, MinHook, D3DCompiler or Visual C++ runtime installation is
required by this build. Windows / DirectX system DLLs are used.

PACKAGE
-------
The ZIP contains exactly these files at root:

QProtocol.asi
QProtocol.ini
README.txt

VALIDATION
----------
Build/CI checks source invariants, PE imports, and package layout. These checks
do not prove a particular GPU/driver/overlay combination works in-game.

Do not promote A24 to stable until the in-game graph index remains stable, F3,
F4, F5-F12, AUTO and Debug Spawn Weapon work again, and A22 clicks, drag,
Insert, resize and Alt-Tab remain stable. Public v0.9.4 remains untouched.
