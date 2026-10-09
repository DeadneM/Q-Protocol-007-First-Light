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


## First Debug TEST implementation candidate

Implementation date: 2026-10-09

Branch: `dev/first-debug`

Base rule: this branch was created directly from tag `v0.9.4`, whose target is
`9439de5f72d1f106362c0fa6033b952b0bb8a07e`. It does not inherit A20K, A20L,
A20M or A20N gameplay experiments.

### Renderer changes

The candidate keeps the existing DXGI / D3D12 / Dear ImGui architecture and
hardens its lifetime rather than disabling the overlay.

- ImGui remains pinned to v1.91.5 and MinHook to v1.3.4.
- The DX12 backend is a project-local hardened derivative of the pinned
  `imgui_impl_dx12.cpp`; the public ImGui API and UI code remain unchanged.
- ImGui vertex/pixel shaders are compiled by CI and embedded in the ASI.
  Runtime `D3DCompile()` was removed.
- `ImGui_ImplDX12_CreateDeviceObjects()` is executed and verified during
  renderer initialization, before `g_overlayReady` becomes true.
- Root signature, pipeline state, font texture/upload resources, temporary font
  upload queue/allocator/list/fence/event, frame upload buffers and map calls
  have explicit failure handling.
- Font upload uses a bounded wait instead of an unbounded wait.
- Unexpected SEH faults while creating device objects, beginning an ImGui frame
  or rendering ImGui draw data are contained at the overlay boundary where
  possible. The renderer is disabled for the session and gameplay continues.
- Present only renders on the tracked game swapchain.
- ResizeBuffers only tears down resources for the tracked game swapchain.
  Resizes from unrelated swapchains are passed through.
- Present and resize/shutdown paths share a renderer mutex.
- Per-frame fence waits are checked before allocator reuse.
- Resource destruction first emits and waits for an overlay queue fence.
  If safe GPU completion cannot be proven and the device is not already
  removed, resources are intentionally retained rather than freed in flight.
- DX12 device-removal reasons and failing HRESULTs are written to QProtocol.log.
- First-open logging distinguishes request, renderer initialization, frame
  construction and successful command submission.
- WndProc restoration checks the live window procedure before writing it back,
  reducing hook-chain damage when another overlay installs after Q Protocol.
- Common overlay modules are reported diagnostically; their presence does not
  disable Q Protocol automatically.

### Queue and swapchain selection

The ExecuteCommandLists hook records the current thread's latest DIRECT queue.
When a candidate Present arrives, First Debug requires a same-device queue and
prefers the DIRECT queue observed on that Present thread. This reduces the risk
of binding an unrelated DIRECT queue in engines or third-party components that
use more than one queue.

The swapchain candidate must belong to the current process, use a visible
top-level window, have a usable client area and expose the same D3D12 device as
the selected queue.

### Dependency hardening

First Debug builds with explicit `/MT`.

CI compiles the ImGui shaders at build time and audits the resulting PE.

Current TEST dependency set:

```text
USER32.dll
KERNEL32.dll
d3d12.dll
dxgi.dll
SHELL32.dll
IMM32.dll
```

The current ASI does not import:

- `D3DCompiler_47.dll`;
- `VCRUNTIME*.dll`;
- `MSVCP*.dll`;
- `ucrtbase.dll`;
- `WriteProcessMemory`.

Dear ImGui and MinHook are compiled into QProtocol.asi. No separate ImGui,
MinHook, D3DCompiler or Visual C++ runtime installation is required by this
candidate.

### CI safety gates

The First Debug workflow checks all of the following:

1. the existing full weapon/config audit;
2. `src/QProtocol.cpp`, `src/GameplayHook.asm`, `src/Overlay.h` and
   `config/QProtocol.ini` have zero diff from tag `v0.9.4`;
3. both ImGui shaders compile successfully at build time;
4. the ASI compiles with the hardened renderer;
5. PE imports/dependencies satisfy the dependency rules above;
6. the ASI still imports expected `ReadProcessMemory` and `VirtualProtect`;
7. the package contains exactly `QProtocol.asi`, `QProtocol.ini` and
   `README.txt` at ZIP root.

Initial implementation CI run #127 / `37975179444` passed all gates.

The only compiler warnings in that run came from the pinned MinHook v1.3.4 C
sources. No First Debug source warning was reported by the build log.

### Validation status

**Do not mark First Debug fixed yet.**

CI proves build, source invariants, package structure and dependency properties.
It does not reproduce the affected users' graphics/driver/overlay environments
and therefore cannot prove the Insert crash is eliminated.

Current status is **TEST candidate / runtime validation pending**. The public
`v0.9.4` release remains untouched until sufficient validation exists.


## Final First Debug TEST build record

The documented candidate was rebuilt after the package README was finalized.

- GitHub Actions: run #128 / `37975713073` — **PASS**
- build commit: `a796879b436d9c35e9fa5fdc8441716bfa750fd4`
- pre-release tag: `first-debug-test`
- asset: `Q-Protocol_First-Debug_TEST.zip`
- QProtocol.asi: 867328 bytes
- QProtocol.asi SHA-256:
  `16f37caee00128f4d464e6f0a483586018a97b1e27d769e42244f8e9d9725729`
- QProtocol.ini SHA-256:
  `8f644b75ff5059efd1acae410c85e9963e847872f0d410ec28d73194a2d27022`
- ZIP: 449150 bytes
- ZIP SHA-256:
  `678e5d75c5fc1ad9210774d49888abe91d441c4c51dc8d3e4c771514a36faa91`

Final comparison against tag `v0.9.4` confirms the protected gameplay/config
files are absent from the branch diff. The branch changes only renderer/build/
documentation surfaces plus the dedicated `src/first_debug/` backend.

The public `v0.9.4` release remains at
`9439de5f72d1f106362c0fa6033b952b0bb8a07e` with the original
`Q-Protocol_v0.9.4.zip` SHA-256
`944f84d4966bb930de31edd976183533f1adb44aeebc10d34401190636132426`.

This record does not change the incident verdict: **runtime validation pending;
not yet declared fixed**.
