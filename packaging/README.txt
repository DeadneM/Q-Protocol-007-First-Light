Q Protocol - Fresh Core A7B
===========================

TEST BUILD

Canonical gameplay base
-----------------------
Fresh Core A5 remains the validated gameplay base.

A7 verdict
----------
Fresh Core A7 is REJECTED.

Reason:
- the first DX12 ImGui integration could enter a visible/open logical state
  before the renderer was actually ready;
- gameplay hotkeys could then be suppressed even though no overlay was visible;
- command allocators were reset without explicit per-frame fence ownership.

A7B correction
--------------
A7B keeps the true in-game DX12 / Dear ImGui direction, but makes it fail-open
and fence-safe.

Gameplay safety:
- OverlayIsVisible() is true only when ImGui/DX12 is genuinely ready.
- If the renderer is not ready, F1-F12 remain active.
- If overlay initialization fails, gameplay remains active.
- If an overlay fence times out, the overlay disables itself and gameplay
  remains active.

DX12 synchronization:
- every swap-chain backbuffer has its own fenceValue;
- a command allocator is reset only after its previous overlay submission has
  completed;
- overlay GPU work is waited before DX12 resources are released;
- the high-frequency ExecuteCommandLists hook is retired after the first DIRECT
  queue is captured.

Diagnostics
-----------
QProtocol.log now reports overlay transitions such as:

DX12 waiting for hooks
DX12 hooks installed; waiting for DIRECT queue
DX12 queue captured; waiting for swapchain/ImGui
DX12 ImGui ready
DX12 ImGui initialization failed (gameplay fail-open)
DX12 overlay fence timeout (overlay disabled; gameplay fail-open)

The goal is that a renderer problem can no longer make Q Protocol itself look
dead.

Overlay / catalogue
-------------------
The A7 interface design is retained:
- true in-game DX12 ImGui rendering;
- Insert toggle;
- Loadout / Weapons / Hotkeys tabs;
- full firearm catalogue;
- role classification;
- Validated vs Experimental weapon status.

Test order
----------
1. Launch the game and confirm F1/F2/F3/F4/F5-F12 still work BEFORE pressing Insert.
2. Press Insert.
3. If the overlay appears, test mouse, tabs, Save, Alt-Tab and level transition.
4. If the overlay does not appear, do not keep pressing Insert: quit normally and
   attach QProtocol.log. The final "Overlay status:" line tells exactly which DX12
   stage failed.
5. Confirm that even if the overlay fails, gameplay hotkeys continue to work.

A7B does not change GiveWeapon or AddAmmo.
