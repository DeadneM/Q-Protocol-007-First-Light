Q Protocol - Fresh Core A8
==========================

DIRECT DXGI / D3D12 RENDERER TEST

Canonical gameplay base
-----------------------
Fresh Core A5 remains the validated gameplay base.

A8 exists only to validate a new true in-game overlay renderer.

A5 vs A7B gameplay audit
------------------------
The repository comparison confirms that the gameplay implementation itself did
not drift between A5 and A7B.

Unchanged:
- all October executable mappings / RVAs
- ResolvePlayer
- License To Kill
- AddAmmo
- GiveWeapon / pair-clone path
- source-graph scan
- 500 ms weapon stabilization
- ManualLoadout
- AutoLoadout
- ManualAmmo
- AutoAmmo
- AUTO one-shot READY logic
- QpGameplayTick
- GameplayHook.asm (identical file SHA)

QProtocol.cpp differs only where the overlay is integrated in WorkerThread:
- include Overlay.h
- overlay bootstrap/status logging
- OverlayPump / reload handling
- suppress F1-F12 only while a renderer-ready overlay is visible
- OverlayShutdown

A8 renderer architecture
------------------------
A7/A7B used Kiero to discover/hook the D3D12 method table.

A8 removes Kiero completely.

Instead it:
1. creates a real DXGI factory only to obtain the system factory vtable;
2. hooks the actual DXGI CreateSwapChain/CreateSwapChainForHwnd methods;
3. captures the game's real ID3D12CommandQueue from the pDevice parameter;
4. hooks Present and ResizeBuffers on the real game swapchain;
5. renders Dear ImGui directly inside that swapchain.

No visible Win32 configuration window is created.

A8 deliberately renders only a compact diagnostic panel. This isolates renderer
validation from the rest of the UI. Once A8 is visible and stable, the full
Loadout / Weapons / Hotkeys interface will be moved onto this renderer.

Controls
--------
Insert = show/hide the A8 renderer test panel.

Fail-open rule
--------------
If the renderer is not ready or fails, A5 gameplay remains active.
F1-F12 are suppressed only while a genuinely initialized in-game overlay is
visible.

Expected QProtocol.log states
-----------------------------
A8 DXGI bootstrap
A8 factory hooks ready; waiting for game swapchain
A8 game swapchain captured; waiting for first Present
A8 DX12 ImGui ready

Failure states:
A8 overlay initialization failed (A5 gameplay unaffected)
A8 overlay fence timeout (overlay disabled; A5 gameplay unaffected)

Test
----
1. Confirm normal A5 gameplay/hotkeys work.
2. Press Insert.
3. Confirm an in-game dark Q PROTOCOL panel appears with no Windows title bar.
4. Confirm mouse interaction works.
5. Close with Insert.
6. Test Alt-Tab and a mission transition.
7. If it does not appear, attach QProtocol.log. The last Overlay status line is
   the important diagnostic.
