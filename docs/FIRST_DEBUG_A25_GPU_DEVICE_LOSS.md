# First Debug — A25/v0.9.6 First-Frame GPU Device Loss

> **A26 experimental test now available (2026-10-11):** source branch `dev/a26-authoritative-swapchain-queue`. A26 records the creating DIRECT queue via DXGI factory swapchain creation hooks and refuses unverified queue substitutions; it also logs `DEVICE_LOST` rather than burying the reason under a synthetic timeout. [A26 TEST ZIP](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/tag/a26-verified-queue-test) is compiled and isolated from v0.9.6, **NOT YET CONFIRMED IN GAME**. See `docs/A26_AUTHORITATIVE_QUEUE.md` on that branch. Incident remains OPEN until runtime verification.

Date: 2026-10-11
Status: **OPEN — corroborated D3D12 device-loss symptom, root command still unknown**
Scope: newest public release **v0.9.6 (A25)** and shared A21 DX12 backend.
Source verified: tag `v0.9.6` `src/OverlayA19.cpp` blob SHA `5bfbc6402fa3f33b95001b8e56f5dfb5c114cac2`.

## User evidence

A public user's log excerpt ends:

```text
Overlay status: Q Protocol A21 DIRECT queue observed; waiting for tracked game Present
[OVERLAY] Init candidate: hwnd=0x00000000001A09AC buffers=3 format=28 size=1920x1080
[OVERLAY] Init complete: shaders/PSO/font ready before first visible frame.
[OVERLAY] Resize begin: buffers=0 size=1920x1080 format=28 flags=0x842
[OVERLAY] Resize complete; renderer will rebuild on the next Present.
Overlay status: Q Protocol A21 DIRECT queue observed; waiting for tracked game Present
[OVERLAY] Init candidate: hwnd=0x00000000001A09AC buffers=3 format=28 size=1920x1080
[OVERLAY] Init complete: shaders/PSO/font ready before first visible frame.
PLAYER READY loadout=0x0000000080B60E50 playerId=201457664
Weapon runtime reset: player identity changed/ready
F1 License To Kill: native state OFF -> FORCE ON
[OVERLAY] A21 cursor clip released while the floating window is open.
[OVERLAY] A21 FirstOpen requested: floating window, selective ImGui input capture.
[OVERLAY] FirstOpen render begin.
[OVERLAY] FirstOpen frame submitted successfully.
[OVERLAY] Render.FrameFence fence reports device removal.
[OVERLAY] Render.FrameFence device removed/reset reason=0x887A002B
[OVERLAY] Render.FrameFence failed: HRESULT=0x800705B4
[OVERLAY] Render.FrameFence device removed/reset reason=0x887A002B
Overlay status: Q Protocol overlay GPU synchronization timeout; gameplay core remains active
```

**Version caveat:** the A21 status-string is reused by the A25/v0.9.6 backend. The excerpt does not contain the executable/ASI version header; *do not infer the installed version solely from its A21 string*. The A25 tagged source was explicitly audited as the latest public baseline.

## Confirmed interpretation (Windows SDK / Microsoft)

- `0x887A002B` = **DXGI_ERROR_ACCESS_DENIED**, not `DXGI_ERROR_DEVICE_REMOVED` or `DXGI_ERROR_DEVICE_RESET`. It is the **reason HRESULT** returned by `ID3D12Device::GetDeviceRemovedReason()`; this HRESULT alone does not identify an offending resource or driver.
- `0x800705B4` = `HRESULT_FROM_WIN32(ERROR_TIMEOUT)`; emitted by the mod's own unsuccessful frame fence path.
- In the actual v0.9.6 source `WaitForFenceValue()` tests `ID3D12Fence::GetCompletedValue() == UINT64_MAX`. Per Microsoft this indicates **device removal**. Therefore the log provides definite evidence the D3D12 device was lost, despite the unusual `DXGI_ERROR_ACCESS_DENIED` reason.
- `FirstOpen frame submitted successfully` logs after `queue->ExecuteCommandLists` and successful `queue->Signal`. GPU command completion was **not** demonstrated.
- Next frame checks the prior `frame.fenceValue` and detects device loss. The code currently converts this failure to timeout even when device loss was already detected; this mislabels the dominant event.
- Shader, PSO, and font creation completed, so old A20I hypothesis of first-open shader compilation failure is **not consistent with this particular trace**.
- Current fail-open disables Q Protocol drawing only. **It cannot guarantee the game survives a D3D12 device already removed**. Avoid claiming that the game remained usable based solely on the status string.

## Concrete source gaps to investigate

1. `BindBestQueueForDevice()` uses `g_tlsLastDirectQueue` (last observed direct queue on Present thread), otherwise `g_commandQueue` (first observed queue), and compares only **device COM identity**. Two queues can share a device. This does **not** prove that the queue was used to create the presenting swapchain.
2. `HookExecuteCommandLists()` remembers raw thread_local queue pointers without owning references for their TLS lifetime; stale pointer access is a possible separate lifecycle concern.
3. `RenderOverlayFrameLocked()` submits a command list directly on the inferred queue with `PRESENT -> RENDER_TARGET -> PRESENT` barriers, without authoritative capture of the swapchain's creating queue. A mismatch in queue ordering or resource state could be hazardous.
4. `DisableOverlayRendering(FenceTimeout, Render.FrameFence, ERROR_TIMEOUT)` currently overrides a genuine device-removal detection with a generic timeout status; distinguish `DeviceLost`, `FenceTimeout`, `WaitFailed` and `QueueSignalFailed`.
5. Check the provenance and lifecycle of the DXGI proxy (A25 ZIP includes native `dxgi.dll`), but the available trace neither establishes nor implicates a loader failure. Do not suggest missing dependencies as the explanation.

## Recommended next implementation, **on a dev branch based exactly on v0.9.6**

- Hook/capture the relevant `IDXGIFactory::CreateSwapChain` and `IDXGIFactory2::CreateSwapChainForHwnd` (plus any other actual factory API used by the game). Per Microsoft, D3D12 `pDevice` argument **is a direct command queue**. Record a reference-counted mapping `swapchain identity -> creation queue -> device`.
- Treat a swapchain without a verified creating queue as *not eligible for overlay draw*, rather than guessing among same-device queues. Never overwrite this authoritative mapping with a thread-local heuristic.
- Audit command-list execution order, backbuffer resource-state barriers, `Present/Present1`, queue Signal and fence frames-in-flight. Use the D3D12 debug layer/GPU-Based Validation and PIX in **developer environments** where available. `ExecuteCommandLists` can trigger device removal on bad resource barriers, reset allocators or reuse of command lists while in-flight.
- Enable DRED auto-breadcrumbs / GPU page-fault reporting **before device creation** only where the environment supports it and without imposing extra setup on public users. Log DRED during an observed device-loss transition when possible, with no private data.
- Improve correctness of error categorization: preserve device loss as the primary event, log HRESULT symbolic name, fence `expected/completed`, queue/swapchain pointer provenance, and last executed call.
- Never solve this by merely increasing 1500 ms to a larger timeout; the observed fence returned `UINT64_MAX`.
- Ensure safe GPU resource retirement, avoid releasing objects still in use, maintain the existing custom DXGI forwarding and export checks.
- Preserve A25 UI sizing, A24 queue safety, A23 weapon graph recovery, A22 mouse fixes, validated F1–F12/Manual/AUTO behavior, INI, and the **four-file root ZIP** `dxgi.dll`, `QProtocol.asi`, `QProtocol.ini`, `README.txt`. Do not overwrite the stable release.

## Acceptance

Internal developer tests include repeat opening/closing, game load/resize, Alt-Tab, fullscreen/borderless, multiple queues/swapchains on same D3D12 device, vendor variations when available, device-removal fault injection, and external overlay coexistence. The public incident remains OPEN until a real fix is validated. Do **not** ask affected end users to perform elaborate experiments.

## Authoritative sources

- Microsoft DXGI errors: https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-error
- Microsoft D3D12 Fence::GetCompletedValue: https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-getcompletedvalue
- Microsoft D3D12 Swap Chains: https://learn.microsoft.com/en-us/windows/win32/direct3d12/swap-chains
- Microsoft CreateSwapChainForHwnd: https://learn.microsoft.com/en-us/windows/desktop/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforhwnd
- Microsoft ExecuteCommandLists: https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-executecommandlists
- Microsoft DRED: https://learn.microsoft.com/windows/win32/direct3d12/use-dred
