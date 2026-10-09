Q Protocol - Fresh Core A21 Overlay Compatibility TEST
=========================================================

STATUS
------
TEST candidate.

A21 pauses the experimental F1 / close-combat research and focuses only on
making the Q Protocol overlay more robust and conventional.

BASE
----
A21 keeps the validated v0.9.4 / Fresh Core A20J gameplay core and includes
the DX12 / ImGui robustness work from First Debug.

A20K-A20N experimental gameplay changes are NOT included.

GAMEPLAY / CONFIG
-----------------
Unchanged from v0.9.4:

- F1 License To Kill behavior from stable A20J
- F2 ammo
- F3 Manual Loadout
- F4 Q-Pistol swap
- F5-F12 weapon slots
- AUTO
- weapon catalogue / RIDs
- timings / retries
- QProtocol.ini format and defaults

A21 development does not continue the three-state F1 experiment.

A21 OVERLAY COMPATIBILITY
-------------------------
The old menu was forced to the center every frame and could not be moved or
resized. It also swallowed every mouse and keyboard message while visible.

A21 changes that model to a conventional single ImGui floating window:

- movable by the title bar
- resizable
- close button in the title bar
- default position/size only on first use
- sensible minimum/maximum size constraints
- no multi-viewport / extra native windows
- no docking dependency

Input handling now follows the standard ImGui model:

- mouse messages are consumed only when ImGui WantCaptureMouse is true
- keyboard messages are consumed only when ImGui WantCaptureKeyboard or
  WantTextInput is true
- other input continues to the game's original WndProc / other hook chain
- hidden overlay does not feed normal game input through ImGui unnecessarily

CURSOR
------
When A21 opens, it releases an active game ClipCursor restriction so the
floating menu can be used normally.

When A21 closes, fails open, or shuts down, it restores the previous cursor
clip state instead of leaving the game cursor globally unclipped.

FIRST DEBUG HARDENING RETAINED
------------------------------
A21 keeps:

- build-time embedded ImGui shaders
- no runtime D3DCompile / D3DCompiler_47.dll dependency
- checked root signature / PSO / font / upload resources
- tracked game swapchain
- scoped ResizeBuffers handling
- checked fences / allocator reuse
- synchronized renderer lifetime
- fail-open renderer behavior
- HRESULT / device-removal diagnostics
- chain-aware WndProc restoration
- static MSVC runtime build

DEPENDENCIES
------------
Dear ImGui v1.91.5 and MinHook v1.3.4 are compiled into QProtocol.asi.

No separate ImGui, MinHook, D3DCompiler or Visual C++ runtime installation is
required by this test build.

PACKAGE
-------
The ZIP contains directly at its root:

QProtocol.asi
QProtocol.ini
README.txt

VALIDATION
----------
A21 is not a public stable release yet. CI validates the build, protected A20J
gameplay/config files, renderer architecture, dependencies and flat ZIP.

Runtime compatibility still needs in-game validation before promotion.
