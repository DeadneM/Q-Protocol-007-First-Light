# A22 — Reliable Mouse Input

Date: 2026-10-09
Branch: `dev/a22-mouse-input`
Base: validated A20J / public v0.9.4 gameplay + First Debug renderer hardening + A21 movable/resizable ImGui window.
Status: **TEST candidate, not validated in game**.

## Trigger

Maintainer reports A21 UI is visually better, but the menu does not consistently respond to mouse clicks. There is no new public crash log or recorded input trace proving a single root cause.

## Two defects found in A21 source

1. `OverlayWndProc` takes `g_renderMutex` with `std::try_to_lock`. If the renderer holds it, ImGui does **not** receive the Win32 input event. Fast mouse down/up pairs can be dropped.
2. A21 forwards Win32 mouse input back to the original game WndProc whenever `io.WantCaptureMouse` is false. That flag reflects the previous ImGui frame and can be stale at the moment of a click. It is inappropriate as the sole arbitration mechanism for an explicitly open modal game-settings overlay.

The game's `WM_INPUT` raw input path may also bypass the standard mouse-message capture decision, allowing simultaneous gameplay actions. This is a compatibility hypothesis, not a proven crash cause.

## A22 implementation

- Replace `std::try_to_lock` in `OverlayWndProc` with a recursive blocking mutex guard, ensuring each Win32 event reaches ImGui, including while Present is active.
- While the UI is visible and backend initialized, forward each Win32 mouse message to ImGui and consume it before the game.
- Keep keyboard capture based on `WantCaptureKeyboard` / `WantTextInput` from A21, and preserve original input routing when the overlay is hidden.
- Suppress the original game's `WM_INPUT` dispatch while the UI is visible, calling `DefWindowProcW` to perform Windows raw-input cleanup.
- Honor the Win32 ImGui backend's `WM_SETCURSOR` handling.
- Add bounded first-three-click log messages: `A22 ImGui mouse-down received #N`.
- A21's title bar, floating single ImGui window, clipping lease, D3D12 resource handling, shaders, fences, and fail-open policy remain intact.
- A20J gameplay files, assembly, public INI, weapon catalogue, timings, F1-F12 behavior, and AUTO remain protected against changes by CI.

## Risks / acceptance

The blocking WndProc guard can delay mouse processing briefly when the renderer is busy. A22 is deliberately a candidate: test for input responsiveness, GPU stalls or input-thread deadlock, and Alt-Tab/resize interactions before promotion.

During open UI, mouse capture is **modal**. Clicking outside the ImGui panel will not activate gameplay until the menu is closed; this is intentional to avoid accidental firing/spawning.

Validation: repeated clicks, checkbox, dropdown, slider dragging, save/reload, title-bar dragging, resizing, close button, repeated Insert toggle, Alt-Tab and borderless, with third-party overlays if available. Do not require end users to perform multi-step diagnostics.

## Release discipline

Test tag: `a22-test` (build confirmation pending).
Public v0.9.4 and the A21 test tag are not overwritten.
ZIP must contain `QProtocol.asi`, `QProtocol.ini`, `README.txt` directly at root.
F1 close-combat experimental research remains paused.
