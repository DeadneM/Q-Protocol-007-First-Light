# A21 — Floating Overlay Compatibility

Date: 2026-10-09
Branch: `dev/a21-overlay-compat`
Status: **TEST candidate — runtime validation pending**

## Direction

F1 / close-combat research is paused.

A21 focuses only on overlay compatibility and carries forward the renderer
hardening from First Debug.

A21 does not include A20K-A20N gameplay experiments.

## Base

Gameplay/config base remains the validated public `v0.9.4 / A20J`.

CI rejects A21 if any of these differ from `v0.9.4`:

- `src/QProtocol.cpp`
- `src/GameplayHook.asm`
- `src/Overlay.h`
- `config/QProtocol.ini`

## UI compatibility changes

The previous overlay window was forced to the center every frame and used:

- `NoTitleBar`
- `NoResize`
- `NoMove`
- `ImGuiCond_Always`

It also swallowed every mouse/keyboard input message while visible.

A21 changes the UI to a conventional single ImGui floating window:

- title bar
- movable
- resizable
- close button
- initial position/size only on first use
- viewport-aware size limits
- no multi-viewport
- no extra native ImGui windows

Input follows the standard ImGui capture pattern:

- mouse is swallowed only when `io.WantCaptureMouse`
- keyboard is swallowed only when `io.WantCaptureKeyboard` or
  `io.WantTextInput`
- uncaptured input continues through the original game WndProc / hook chain
- when hidden, normal game input is not unnecessarily fed to ImGui

## Cursor compatibility

A21 tracks the game's cursor clip state.

When the overlay opens, an active `ClipCursor` restriction is released so the
floating menu can be used normally.

When the overlay closes, fails open, or shuts down, the previous clip state is
restored instead of leaving the game cursor globally unclipped.

## First Debug hardening retained

A21 keeps:

- build-time embedded ImGui shaders
- no runtime `D3DCompile()`
- no `D3DCompiler_47.dll` dependency
- checked DX12 root signature / PSO / font resources
- checked upload/map/command-list/fence paths
- tracked swapchain
- scoped ResizeBuffers handling
- serialized renderer lifetime
- fail-open renderer behavior
- HRESULT/device-removal diagnostics
- chain-aware WndProc restore
- explicit `/MT` build

## CI result

GitHub Actions run:

- run #131 / `37979873788`
- result: **PASS**

Checks passed:

- weapon/config audit
- A20J gameplay/config invariant
- A21 floating-overlay architecture audit
- build-time shader compilation
- ASI compile
- PE dependency audit
- flat ZIP audit
- A21 test pre-release publication
- stable v0.9.4 release guard

## Binary / package

```text
QProtocol.asi
  size:    868352 bytes
  SHA-256: c98e11df2b7f5a7a15a6a3839343b364844e7d9d99f03b76b04ded29afa45a66

QProtocol.ini
  size:    6717 bytes
  SHA-256: 8f644b75ff5059efd1acae410c85e9963e847872f0d410ec28d73194a2d27022

Q-Protocol_FreshCore_A21_OverlayCompatibility_TEST.zip
  size:    450038 bytes
  SHA-256: 266df5e4abb3d6ed00480fdc6b3898b2f155bd4c70a354259bdc68d1531d7ae8
```

Pre-release:

`a21-test`

## PE dependencies

```text
USER32.dll
KERNEL32.dll
d3d12.dll
dxgi.dll
SHELL32.dll
IMM32.dll
```

No separate ImGui, MinHook, D3DCompiler or dynamic VC++ runtime installation is
required.

## Validation limit

A21 is not yet a validated public fix.

CI proves source invariants, build, dependencies and package structure. It does
not prove compatibility with every GPU/driver/third-party overlay ordering.

The public stable release remains `v0.9.4` until sufficient runtime validation.
