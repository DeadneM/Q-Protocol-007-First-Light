# First Debug — Public Overlay Crash on Insert

Date: 2026-10-09
Project: Q Protocol / 007 First Light
Incident ID: **First Debug**
Status: **OPEN — compatibility incident; root cause not yet proven**
Initial affected build reported: **v0.9.3 / Fresh Core A20I**.
Current validated gameplay baseline: **v0.9.4 / Fresh Core A20J**.
Priority: **public-user stability**.

## User reports

- A user reports that 007 First Light crashes **when pressing Insert to open Q Protocol's overlay**.
- The maintainer reports that other users appear to encounter the same problem.
- Users should **not** be asked to perform intrusive or multi-step troubleshooting, change hotkeys, install random DLL packs, turn off antivirus, or test experimental builds to diagnose this.
- Handle compatibility diagnosis, reproduction, fixes and validation on the **maintainer/development side**.

## Evidence from the supplied A20I log

The original log shows:

```text
Q Protocol v0.9.3 / Fresh Core A20I Native LTK State Toggle
EXE TimeDateStamp = 0x6AC4D653
EXE SizeOfImage = 0x06D31000
Target executable accepted.
Gameplay hook installed at EXE+0x194BC01.
LTK native-state observe hook installed at EXE+0x191A6D8.
Overlay status: A19 DIRECT queue captured; waiting for game Present
Overlay status: A19 DX12 ImGui ready
Overlay status: A19 DIRECT queue captured; waiting for game Present
Overlay status: A19 DX12 ImGui ready
PLAYER READY loadout=0x000000002E073540 playerId=201457664
Weapon runtime reset: player identity changed/ready
```

- The EXE checks, main gameplay hook, LTK observe hook, DX12 queue capture, ImGui backend initialization, and player readiness reached their reported success stages.
- `Auto Enabled = 0` in this log; no weapon spawn is evidenced before the journal ends.
- Neither an exception code nor a faulting module/address is present; the last log line **is not proven** to be the crash site.
- The repeated QueueCaptured -> ImGuiReady state can follow a ResizeBuffers/reinitialization cycle; **not proof on its own** of a bug.

## Dependencies confirmed from the v0.9.3 build workflow

- Dear ImGui **v1.91.5** and MinHook **v1.3.4** are compiled directly into `QProtocol.asi`; end users do **not** install these separately.
- MSVC project builds `/LD` without `/MD` in the workflow, targeting the default static C/C++ runtime configuration; confirm final PE imports in release QA.
- Windows/DX12 components: `d3d12.dll`, DXGI and `D3DCompiler_47.dll` through the pinned ImGui DX12 backend shader compilation.
- The game's existing ASI loader is necessary to load the ASI, but loading is already confirmed by the log.
- `QProtocol.ini` belongs at the ZIP root alongside `QProtocol.asi` and the README; retain that public packaging invariant.
- There is presently **no evidence** that installing the Visual C++ Redistributable or separate ImGui/MinHook DLLs would cure this crash.

## Why the first Insert press matters (source audit)

In v0.9.3 `src/OverlayA19.cpp`:

1. `OverlayPump()` sees the edge on `[Overlay] ToggleKey` (Insert by default), sets `g_requestedVisible=true`, and marks UI data for reload.
2. `HookPresent()` previously bypasses drawing while invisible; on visibility it invokes `ImGui_ImplDX12_NewFrame()`, the Win32 ImGui frame, `DrawOverlayWindow()`, and `ImGui_ImplDX12_RenderDrawData()`, then submits graphics commands.
3. `InitializeImGuiForSwapChain()` previously reported `ImGuiReady` after `ImGui_ImplDX12_Init()`. In pinned ImGui **v1.91.5**, `ImGui_ImplDX12_Init()` allocates backend state but does not create its full D3D12 pipeline; the first `ImGui_ImplDX12_NewFrame()` may call `ImGui_ImplDX12_CreateDeviceObjects()`.
4. Pinned `ImGui_ImplDX12_NewFrame()` **does not propagate `CreateDeviceObjects()` failure**. The mod does not gate the subsequent render on proof that shader/root-signature/PSO/font resources are fully ready. This is a concrete robustness gap and a particularly relevant **hypothesis**, not yet a demonstrated root cause.
5. `HookPresent()` uses global `g_swapChain3` render targets without always confirming the incoming Present belongs to that swapchain.
6. `HookResizeBuffers()` calls `ShutdownDx12Backend()` without checking whether the resized swapchain is the tracked game swapchain.
7. `ShutdownDx12Backend()` calls `WaitForAllFrames(2000)` but proceeds to release GPU resources even on a failed wait; no clear full synchronization exists between the Present and ResizeBuffers hook paths. Potential resource race.
8. These render-lifecycle gaps predate A20I (also in A20F); do not assume any one feature change exclusively introduced the issue.

## Required developer-side fix plan

**Maintain the validated A20J gameplay architecture.** Leave F1-F12, native weapon graphs/RIDs, Manual/AUTO sequencing, ammo behavior, INI defaults and in-game UX intact.

1. Treat shader compilation, D3D12 root signature, PSO, font atlas upload and each relevant GPU resource creation as fallible. Verify success **before** marking the renderer usable; if the backend cannot initialize, do not call its render functions.
2. Make the DX12 overlay *fail open*: disable overlay rendering only and preserve the underlying gameplay/game when graphics initialization fails.
3. Bind `Present`, `ResizeBuffers`, frame resources and command queue to the correct swapchain/device and handle swapchain replacement.
4. Synchronize GPU resource lifetimes. Do not free/recycle resources still in use when fence waiting fails; do not render concurrently with resize/shutdown.
5. Add concise, noncumulative stage/error logging (`Init`, `FirstOpen`, `NewFrame`, `CreateDeviceObjects`, `DrawUI`, `RenderDrawData`, `Execute`, `Resize`) and HRESULT/failed API when available; never log private user data.
6. Add automated build/package/PE import checks. Internally test both failure paths and successful repeated opens/closes, fullscreen/borderless, resize, Alt-Tab, differing GPU vendors/driver combinations and coexistence with common overlays where available.
7. Do **not** declare fixed solely because GitHub Actions compiles or one PC succeeds; no public release without adequate runtime validation.

## Acceptance criteria

- Pressing Insert opens/closes Q Protocol without terminating the game, including repeated use and after display changes.
- If overlay rendering cannot be initialized safely, gameplay remains functional and the overlay fails gracefully.
- Gameplay behavior and hotkey/weapon settings are unchanged.
- Default package remains flat: `QProtocol.asi`, `QProtocol.ini`, `README.txt` at ZIP root.
- This investigation remains recorded under the persistent project name **First Debug** until resolved, with evidence-based status updates and a reproducible fix.

## Project rule

**Developer owns the problem.** No reliance on customers to swap key bindings, inspect Windows crash reports, disable security software or carry out complex experiments as the normal path to resolving the public crash. If any user volunteers a crash log, it may help, but it is not a prerequisite for project progress.

## References

- `src/OverlayA19.cpp` at tag `v0.9.3`
- `.github/workflows/build-fresh-core.yml` at tag `v0.9.3`
- ImGui DX12 backend pinned to `ocornut/imgui v1.91.5`
- `PROJECT_STATE.md`, `docs/STATUS.md`


## A21 continuation

First Debug established the DX12/ImGui hardening base, but overlay usability and
compatibility work continues in **A21**.

A21 keeps First Debug's renderer fixes and changes the UI/input model:

- floating movable/resizable single ImGui window;
- title bar and close button;
- no forced center/size every frame;
- selective `WantCaptureMouse` / `WantCaptureKeyboard` input swallowing;
- cursor clip state restored on close/failure/shutdown.

F1 / close-combat experiments are paused and are not part of A21.

A21 CI run #131 passed. Runtime validation is still required before declaring
the public Insert crash resolved.

See `docs/A21_OVERLAY_COMPAT.md`.


## A22 follow-up: mouse-click reliability (2026-10-09)

A21 substantially improves window presentation, but the maintainer reports that mouse clicks are still unreliable. This does **not** prove First Debug's original Insert crash has been fixed.

The A21 source revealed two mouse-path problems: `OverlayWndProc` used `std::try_to_lock` (dropping messages on renderer-lock contention), and used frame-stale `WantCaptureMouse` for game/ImGui click arbitration. Gameplay raw input via `WM_INPUT` also remained able to bypass those normal mouse messages.

A22 (`dev/a22-mouse-input`) corrects event delivery, gives the visible menu modal ownership of Win32 mouse input, and routes `WM_INPUT` through `DefWindowProcW` for Windows cleanup rather than to the gameplay WndProc while open. A20J gameplay, the stable INI and First Debug graphics hardening remain protected.

CI #133 / `37984376129` passed. A22 ZIP SHA-256 `41ac99b5186c507f1566e730d57489edf7ce60195705d415b42071e38e8966c7`. Candidate release: `a22-test`.

**Incident status remains OPEN** pending runtime tests of Insert, mouse clicking, dragging, toggling, Alt-Tab, and mixed third-party overlays. Do not label fixed based only on CI. Full details: `docs/A22_MOUSE_INPUT.md`.
