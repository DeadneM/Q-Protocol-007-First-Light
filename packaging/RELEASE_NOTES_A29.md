# Q Protocol A29 - Updater and readable menu TEST

Based on A28, reported working by the maintainer on 2 November 2026. The fresh
A28 log confirms player READY, the complete three-role F3 loadout, F1 transitions
and native F2 publication for all six ammo classes. This is validation on that
configuration, not a universal GPU compatibility claim.

## Updates inside the mod menu

- **Check updates at start:** one asynchronous GitHub check, enabled by default.
- **Beta builds:** stable-only when OFF; stable and TEST releases when ON.
  Default ON in this experimental build, matching the Darksiders TEST workflow.
- **Check now** and **Update on exit**, with confirmation and scheduled-update
  cancellation. The game is never closed or relaunched by the updater.
- Only a newer release declaring compatibility with the installed EXE is offered.
  Old packages without the new manifest are ignored, so stable v0.9.6 cannot
  accidentally replace the November-compatible core.
- Downloads, SHA-256/AMD64/ZIP checks and file replacement run in an independent
  helper. It waits on the actual game process, then checks for another session.
  Personal INI/README remain untouched; previous binaries are backed up under
  `Retail/QProtocol-Updates`. A failed partial replacement restores old binaries.

## Packaging policy approved by the maintainer

The ZIP now contains **only `dxgi.dll` and `QProtocol.asi` at root**. README,
release notes and default configuration remain on GitHub. The default INI is
embedded in the ASI and created only if missing; an existing INI is never reset
by installation or update. The updater helper is also embedded in the ASI.
Windows .NET Framework 4.x runs the extracted helper.

Install with the game closed: back up the installed two binaries and INI, then
extract the ZIP beside `Retail/007FirstLight.exe`. Preserve your existing INI.
To roll back, close the game and restore the two saved binaries. Older mod builds
still require a matching game EXE.

## Menu

Loadouts adapt to narrower windows; help text has better contrast; player status
occupies its own line. Controls, Updates and Advanced sections are clearly
separated. Save / Reload / Defaults remain outside the scrolling content.
Native gameplay, F1-F12, AUTO, renderer, input and compatibility gates stay A28.

## Test boundary

This is **TEST**, not a new stable promotion. Windows build, fixture transaction
tests and CPU ImGui smoke checks do not validate gameplay or the real post-exit
update cycle. Maintainer test: startup check, channel switch, menu readability,
then a future compatible update staged and applied after normal game exit.
Stop on any unexpected result and keep both mod logs and updater status files.
The independent GPU DEVICE_LOST issue remains open.
