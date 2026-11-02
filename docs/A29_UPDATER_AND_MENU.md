# A29 updater and menu - 2026-11-02

## Base and user decisions

- A28 source/tag `580e57cb1e240df04e1a3f8cdc0fb0b46067fcc2`.
- Installed from a verified package, with backup and personal INI preserved.
- Maintainer: "ok parfait tout a lair de marcher". Fresh logs are archived in
  `evidence/a28-runtime/`: READY, F1 OFF/ON, 41 linked graphs, complete F3 and F2
  six-class native publication. This accepts A28 on the maintainer configuration.
  Broader hardware validation and the other user's GPU failure remain open.
- New development branch: `dev/a29-updater-menu`; no `main` promotion.
- The maintainer explicitly superseded the older four-file ZIP rule: future ZIPs
  contain the two binaries only; README/default INI/history stay on GitHub.
- Reference inspected read-only: Darksiders Genesis `work-debug/src/UpdateFeature`
  and `OverlayUi.cpp` Updates tab. Same user-facing pattern: startup check, beta,
  explicit installation after exit, retained settings and backups.

## Architecture

`src/Updates.cpp` is separate from gameplay. A pinned ASI worker extracts an
embedded managed helper and starts it hidden. Rendering only reads a locked status
snapshot and enqueues actions. GitHub access, hashing, ZIP parsing, waiting and
installation execute outside render/gameplay threads. No native quit hook,
process termination, System32 change or automatic game relaunch is added.

`updater/UpdateEngine.cs` selects published release assets with a strict
`QProtocol-update.json` schema. Stable ignores prereleases; beta includes them.
Only manifests with a larger integer build and the exact current EXE SHA-256
qualify. Known names are `dxgi.dll` / `QProtocol.asi`; URLs are pinned to this
repository's release/tag, with HTTPS-only allowed GitHub asset redirects.
Archive and individual binary size/SHA-256 are checked, along with AMD64 DLL PE
headers, two-entry root layout and duplicate/traversal rejection.

`Update on exit` rechecks release selection against the build the user confirmed,
probes target-directory write access and stages locally. The helper keeps a handle
to the original parent, supports cancellation while waiting and checks for another
game session before applying. Source hashes are captured before waiting and
rechecked after exit. Backups are verified before replacement; failure after the
first replacement restores changed files. INI and README are never payload files.
No downgrade is performed on a channel switch.

State files and extracted helpers are under `%LOCALAPPDATA%/QProtocolUpdater`.
`status.txt` records checking, available, downloading, staged, cancelled, installed
or error. Backups remain under `Retail/QProtocol-Updates/before-*`; `backup.json`
records previous binary hashes. Restoring two old binaries is manual with the
game closed. No automatic helper elevation is attempted; write denial is reported.

`QProtocolResources.rc` embeds the helper (101) and authoritative source INI (102).
The core uses CREATE_NEW to seed a missing INI after EXE validation and before
loading settings. Existing bytes are preserved. New updater options are read with
defaults and written only when changed: `[Updater] CheckAtStart=1`, `Beta=1`.
Profile Save/Reload/Defaults retain their previous semantics; updater preferences
save immediately, as labelled in the Updates tab.

## Scope and acceptance

`tools/verify_a29_scope.py` compares against A28. Core changes are limited to
bootstrap/update lifecycle and build text. Native map/gates, A27 scanner, A24
request guards, AUTO, F1-F12, assembly, INI, native DXGI and DX12/input stay frozen.
Presentation changes are restricted to DrawLoadoutTab, DrawOverlayWindow,
ApplyStyle and the new Updates tab. DrawFooter remains unchanged.

`tests/UpdaterTests.cs` covers manifest/channel/version/compatibility rejection,
package integrity, x86/traversal rejection, partial-install rollback, temporary
cleanup, old-module refusal and byte-exact INI/README preservation using fixtures.
`tests/MenuSmoke.cpp` constructs the actual ImGui menu at three viewport sizes,
the Updates controls, embedded-default creation and existing-INI reuse. It uses
no game, GPU, window or network. These checks are not runtime gameplay proof.

Success still requires maintainer visual acceptance and one real future update:
check at start, stable/beta switch, confirm, wait while the game remains open,
cancel once, then stage and close normally. Inspect updater status, two binary
hashes, untouched INI, retained backups and next-launch version. Do not manufacture
an artificial newer live release merely to claim this runtime test passed.

## TODO

- DONE: Windows CI 41436950246 SUCCESS, including helper/resource/ASI/DXGI/menu/package checks.
- DONE: downloaded two-binary package, SHA-256, embedded helper and exact default INI checked; live public discovery/download probe passed.
- Actual in-game menu, startup check and post-exit update test pending.
- Stable channel becomes available only when a compatible manifest is deliberately
  released as stable. Existing incompatible v0.9.6 remains immutable.

### A29 build and published-package verification - 2026-11-02

**CONSTRUIT / CONTROLE / EXPERIMENTAL.** Windows CI
[41436950246](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/41436950246)
SUCCESS on source `e04b7de7d7b529e0353ce241befe62a5be2e5504`: native DXGI export/
forwarding smoke, preservation audits, 19 updater transaction checks, embedded
helper/default-INI resource compile, ASI, actual ImGui CPU smoke at 3 viewport
sizes and dependency/two-file package checks. The earlier CI attempt stopped at
legacy CSC slash paths; correction was limited to the workflow, with no release
from the failed attempt.

[A29 TEST ZIP](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/download/a29-updater-menu-test/Q-Protocol_v0.9.6_A29_UpdaterMenu_TEST.zip):
584139 bytes; SHA-256
`78bfaf255013c138020ffad4a4fcae93286797af023cf2ec20183e44a2448484`.
Downloaded package independently checked: CRC, two exact root binaries, AMD64,
individual hashes, A29 identity and embedded default INI identical to the baseline
(`8f644b75ff5059efd1acae410c85e9963e847872f0d410ec28d73194a2d27022`).
Updater helper is embedded; README and INI are absent from the ZIP as requested.

The actual updater engine found the compatible A29 beta, downloaded/verified its
published package, excluded it on stable and did not offer it again to build 29.
This was a read-only public-network probe with extraction into the workspace,
not a real game update. Probe source: `evidence/PublishedUpdateProbe.cs`; complete
hashes/results: `evidence/a29-verification.json`.

A28 remains installed and accepted locally. A29 was NOT installed or launched.
Next: maintainer visual/startup/channel test, then an actual staged update after
exit when a future compatible build exists. No stable promotion; GPU incident
on the other system remains open. Old main/tag/release archives remain unchanged.

