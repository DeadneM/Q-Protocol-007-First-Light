# First Debug — DX12 / ImGui Implementation Audit

Date: 2026-10-09
Branch: `dev/first-debug`
Stable base: `v0.9.4` / A20J (`9439de5f72d1f106362c0fa6033b952b0bb8a07e`)
Status: **TEST candidate — runtime validation pending**

## Scope lock

This compatibility branch changes the overlay renderer only.

CI rejects the build if any of these differ from `v0.9.4`:

- `src/QProtocol.cpp`
- `src/GameplayHook.asm`
- `src/Overlay.h`
- `config/QProtocol.ini`

Therefore First Debug does not intentionally change F1-F12, License To Kill,
native weapon graphs, ammo, Manual Loadout, AUTO, RIDs, timing, retry behavior
or INI semantics.

## A20J renderer audit

The stable renderer had several concrete robustness gaps.

### Deferred first-open pipeline creation

`ImGui_ImplDX12_Init()` in pinned ImGui v1.91.5 only allocated backend state.
Root signature, shaders, PSO and font resources were deferred until the first
`ImGui_ImplDX12_NewFrame()`.

A20J marked the renderer ready immediately after `ImGui_ImplDX12_Init()`.
The first visible frame therefore entered UI/render work before Q Protocol had
proof that `ImGui_ImplDX12_CreateDeviceObjects()` succeeded.

### Backend failures hidden from the host

Pinned `ImGui_ImplDX12_NewFrame()` called
`ImGui_ImplDX12_CreateDeviceObjects()` when needed but discarded its boolean
failure result. Several font-upload operations used assertions rather than a
recoverable failure path.

### Swapchain scope

A20J stored one global `g_swapChain3` but HookPresent did not always prove that
the incoming Present belonged to it before indexing its back buffers.

HookResizeBuffers tore down the DX12 backend for every hooked ResizeBuffers
call, including an unrelated swapchain.

### GPU lifetime

A20J waited up to two seconds before backend destruction, but released resources
even if the wait failed. That can free command allocators/render targets while
the GPU may still reference them.

Present and ResizeBuffers also lacked a shared renderer-lifecycle lock.

### Queue selection

The first DIRECT queue observed globally was latched. A process can expose more
than one DIRECT queue, particularly with engine subsystems or injected overlays.

### Dependencies

The stock ImGui v1.91.5 DX12 backend compiled HLSL with `D3DCompile()` at
runtime, introducing `D3DCompiler_47.dll` as an ASI dependency.

No evidence supported a missing ImGui/MinHook installation or a random external
DLL pack as the public crash cause.

## First Debug architecture

### Build-time shaders

The ImGui VS/PS sources live in:

- `src/first_debug/imgui_vs.hlsl`
- `src/first_debug/imgui_ps.hlsl`

CI compiles them with Windows SDK FXC and embeds the bytecode into QProtocol.asi.

The hardened backend contains no runtime `D3DCompile()` call.

### Hardened ImGui DX12 backend

`src/first_debug/imgui_impl_dx12_first_debug.cpp` is derived from the pinned
ImGui v1.91.5 backend.

It adds recoverable HRESULT/stage reporting and checks:

- root-signature serialization;
- root-signature creation;
- PSO creation;
- font texture creation;
- upload buffer creation and Map;
- temporary font queue/allocator/list/fence/event;
- command-list Close;
- queue Signal;
- SetEventOnCompletion;
- bounded font upload wait;
- ImGui vertex/index buffer creation;
- vertex/index Map.

Partial resources are cleaned on the checked failure paths.

### Initialization contract

First Debug does not publish `g_overlayReady=true` until:

1. the game swapchain/device/queue candidate is accepted;
2. all Q Protocol frame resources are created;
3. the Win32 ImGui backend is initialized;
4. the DX12 ImGui backend is initialized;
5. `ImGui_ImplDX12_CreateDeviceObjects()` succeeds;
6. shader/root-signature/PSO/font resources are therefore ready.

This moves the dangerous deferred work away from the first visible UI draw.

### Present path

Present is serialized through the renderer mutex.

The hook:

- ignores unrelated swapchains;
- waits the selected frame fence before allocator reuse;
- checks allocator reset;
- checks command-list reset;
- guards ImGui NewFrame/UI construction;
- checks renderer backend error state;
- guards RenderDrawData;
- checks command-list Close;
- submits only after all previous stages succeed;
- checks queue Signal;
- records a per-frame fence value only after successful Signal.

A failed renderer stage turns off overlay rendering for the session rather than
changing gameplay state.

### ResizeBuffers path

Only the tracked swapchain triggers renderer teardown.

The resize path:

1. hides the overlay;
2. serializes against Present;
3. signals and waits for Q Protocol GPU work;
4. destroys DX12 resources only when GPU completion is proven or the device is
   already removed;
5. calls the original ResizeBuffers;
6. rebuilds lazily on a later valid Present.

If safe completion cannot be proven, resources are retained instead of being
released in flight and the renderer fails open.

### WndProc coexistence

First Debug checks the current WndProc before restoring Q Protocol's predecessor.
If another component replaced the chain after Q Protocol, First Debug does not
overwrite that later hook.

WndProc input handling uses a non-blocking renderer lock. If a resize/shutdown
owns the renderer lock, input is passed through instead of touching ImGui state
concurrently.

### Multiple DIRECT queues

The ExecuteCommandLists hook records the most recent DIRECT queue per thread.
A Present candidate prefers the DIRECT queue observed on that Present thread and
requires device identity to match.

### Diagnostic logging

Useful stages include:

- renderer bootstrap;
- common overlay module presence;
- queue binding;
- swapchain init candidate;
- each important DX12 resource/API failure;
- ImGui CreateDeviceObjects stage/HRESULT;
- first Insert/open request;
- first visible render begin;
- first successful submitted frame;
- Resize begin/end;
- fence timeout;
- DX12 device-removed reason.

No user-sensitive data is logged.

## Overlay coexistence considerations

First Debug does not assume Steam/Discord/RTSS/OBS/NVIDIA overlays are absent.
Known modules are reported for context only.

MinHook install failures are handled fail-open and partial hook creation is
cleaned. The build does not disable or patch another overlay.

Hook-chain coexistence can never be guaranteed for every injector because two
products may both patch the same DXGI entry point with incompatible techniques.
This remains a runtime compatibility limit, not a reason to disable Q Protocol's
overlay functionality.

## Dependency audit

Initial run #127 reported:

```text
USER32.dll
KERNEL32.dll
d3d12.dll
dxgi.dll
SHELL32.dll
IMM32.dll
```

Absent by CI rule:

- `D3DCompiler_47.dll`
- `VCRUNTIME*.dll`
- `MSVCP*.dll`
- `ucrtbase.dll`
- `WriteProcessMemory`

The build is explicit `/MT`.

Dear ImGui and MinHook are compiled into QProtocol.asi. No separate installation
of either library is required.

## Build validation

Initial implementation CI:
- run #127 / `37975179444`
- result: PASS
- gameplay/config diff gate: PASS
- shader compilation: PASS
- ASI build: PASS
- PE dependency audit: PASS
- flat ZIP audit: PASS
- pre-release publication: PASS

The compiler emitted warnings from the pinned MinHook v1.3.4 C sources. The
First Debug project source did not emit a warning in that run.

## Remaining validation limits

The following are not proven by CI:

- whether the original affected-user Insert crash is eliminated;
- all GPU vendors/driver versions;
- all borderless/fullscreen/Alt-Tab/resize sequences;
- every third-party overlay hook ordering;
- device-lost behavior on real hardware.

Therefore this is a **TEST compatibility candidate**, not a declared public fix.

The stable public release stays `v0.9.4` until sufficient runtime evidence is
available.
