Q Protocol - Fresh Core A22 Reliable Mouse Input TEST
=====================================================

STATUS
------
TEST candidate. Mouse-click reliability is not declared validated in-game yet.
Public stable remains v0.9.4 / A20J.

BASE
----
A22 builds on the A21 floating ImGui menu, First Debug DX12 hardening, and the
validated v0.9.4 / Fresh Core A20J gameplay core. Experimental F1 branches
A20K-A20N are excluded; close-combat / F1 research is paused.

A22 MOUSE INPUT CORRECTIONS
---------------------------
- The Win32 WndProc no longer skips UI mouse events when the renderer mutex
  is temporarily occupied. A21 used try_to_lock, which could lose clicks.
- While the menu is visible, Win32 mouse events are delivered to ImGui and
  swallowed before they reach the game. A21's WantCaptureMouse gate could be
  stale at the instant of a click.
- WM_INPUT is passed to DefWindowProc for required Windows raw-input cleanup,
  but not to the game's original WndProc while the menu is open.
- Win32 cursor messages handled by ImGui retain the backend's return value.
- First three left-button-down deliveries are logged as
  "A22 ImGui mouse-down received" for local diagnosis.
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

Do not promote A22 to stable until runtime clicking, dragging, sliders,
checkboxes, dropdowns, save/reload, repeated Insert opens/closes, and Alt-Tab
are validated. Public v0.9.4 remains untouched.
