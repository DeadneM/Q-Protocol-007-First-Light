# A25 — Accessible default overlay layout

Date: 2026-10-10
Branch: `dev/a25-overlay-fit-default`
Base: exact stable v0.9.5 / A24 gameplay, first debug DX12 renderer, A22 mouse input.
Status: TEST candidate pending user validation.

## User report and screenshot

On 3840×2160 the floating Q Protocol menu opens at a compact 900×650 ImGui units. In Loadout it has two fixed 485-unit-high panels; adding its header, tabs, experimental visibility checkbox and footer buttons exceeds the initial height. The buttons are reached only after manually scrolling the main window, which ordinary users may not discover.

## A25 scope

Modify only `DrawOverlayWindow()` in `src/OverlayA19.cpp`:

- Default initial window size **1000×850** ImGui units, bounded by viewport work area minus 32 units on each axis.
- Keep centered first-use window location and **ImGuiCond_FirstUseEver**. User is still free to move and resize.
- Use a dedicated vertical scrolling child for the active Loadout/Weapons/Debug/Hotkeys tab content.
- Reserve 78 units **outside** that child for `DrawFooter()`: Save / Reload / Defaults buttons remain visible even when the window is resized small.
- On high-resolution 4K/1440p/1080p, the default Loadout content should fit without vertical scrolling; at small viewport heights, tab content can scroll independently while the action row remains visible.

Keep the Loadout panel sizing, user INI contents, 31-weapon catalogue, gameplay hooks, native LTK/ammo/spawn/AUTO, overlay DX12 renderer, mouse handling, and optional standalone DXGI proxy completely unchanged.

## Packaging

Separate test ZIP `Q-Protocol_v0.9.5_A25_OverlayFit_TEST.zip` with `QProtocol.asi`, `QProtocol.ini`, `README.txt` **directly at the ZIP root**. No `dxgi.dll` inside; the user can choose their previously tested loader independently.

## CI / runtime verification

- Source audit enforces byte-identical gameplay/INI in text after line ending normalization; only UI function may change.
- Checks new default bounds, child/footer order and compile.
- User to validate that all controls, especially Save / Reload / Defaults, are visible at first opening; then validate smaller resize behavior, Insert and mouse/drag, and F1–F12/AUTO are unaffected.
- This is **not** a promotion of the public v0.9.5 until runtime test is confirmed.

