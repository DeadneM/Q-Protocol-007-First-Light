# A26 First Debug — authoritative swapchain queue / GPU-loss classification TEST

Date: 2026-10-11
Base: public stable **v0.9.6 / A25**
Status: **EXPERIMENTAL, requires runtime validation**. Never promote to public release without game testing.
Context: `docs/FIRST_DEBUG_A25_GPU_DEVICE_LOSS.md`.

## Incident reproduced in public user log

- First overlay frame submitted and queue Signal returned success.
- On next frame, `Render.FrameFence` observed `GetCompletedValue()==UINT64_MAX` (D3D12 device lost).
- `GetDeviceRemovedReason()==0x887A002B` (`DXGI_ERROR_ACCESS_DENIED`).
- Prior build misleadingly reported synthetic `0x800705B4` timeout.
- Diagnosis remains a hypothesis until a runtime trace and/or driver validation confirms the command causing removal.

## A26 experimental correction

- Hook `IDXGIFactory::CreateSwapChain` and `IDXGIFactory2::CreateSwapChainForHwnd` at DXGI factory vtable entry points [10] and [15], respectively, alongside existing DirectX12 Present/Resize/Execute hooks.
- At successful swapchain creation, obtain the direct command queue from D3D12 factory `pDevice`, verify DIRECT type, and associate COM swapchain identity with an AddRef-owned queue.
- At first accepted game Present, accept **only the queue authoritatively recorded for the same swapchain**, verifying it belongs to the game's D3D12 device. Never substitute `g_tlsLastDirectQueue`, a same-device queue or the first queue previously observed.
- Unknown or unsupported swapchain creation paths **fail open**: no Q Protocol GPU commands are submitted to that swapchain. Gameplay remains active; overlay may be unavailable until a supported DXGI creation path is captured.
- Queue-binding map bounded to 16; COM queue references released on replacement and shutdown. Weak swapchain identity keys are never dereferenced after QI; normal swapchain release is not obstructed by Q Protocol.
- `HookExecuteCommandLists` is retained for diagnostics but no longer captures an unowned thread_local raw pointer or changes the rendering queue.
- Frame-fence device removal becomes distinct `OverlayStage::DeviceLost` with the actual driver HRESULT, expected fence value, and active queue/swapchain pointers. A timeout remains a timeout only when the fence did not confirm a device removal.
- Preserve the existing fail-open renderer behavior; **device loss cannot guarantee the game survives**.

## Frozen behavior

- **Identical to v0.9.6**: `src/QProtocol.cpp`, `src/GameplayHook.asm`, `src/Overlay.h`, `config/QProtocol.ini`, shader and first_debug backend, all F1-F12/Manual/AUTO, A24 queue guard, A23 graph scanner.
- A25 1000x850 window, pinned bottom Save/Reload/Defaults and A22 cursor control remain untouched.
- Custom DXGI x64 proxy is rebuilt and tested by CI and remains included by default in the **four-file flat ZIP**: `dxgi.dll`, `QProtocol.asi`, `QProtocol.ini`, `README.txt`.

## Acceptance / remaining risk

- CI must verify 31-weapon catalog, source preservation, relevant DXGI hooks, present-command selection isolation, no TLS raw queue, DXGI exports/ordinal forwarding, CreateDXGIFactory1 smoke, PE imports, and ZIP root.
- On affected hardware, both `QProtocol.log` and `QProtocolDXGI.log` are helpful. Look for `A26 verified swapchain creation`, `A26 authoritative PRESENT queue bound`, `FirstOpen frame submitted successfully`, and no `DEVICE_LOST` after repeated Insert opens/resize/Alt-Tab.
- A26 may decline overlay drawing if the game's swapchain was created before hooks or through another DXGI factory method; this is intentional until more evidence. Never force fallback to another queue.
- This implementation is **not yet proven to correct the public reporter's failure**. Do not request complex experiments from affected end users.
