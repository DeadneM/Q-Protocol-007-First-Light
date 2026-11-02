## 2026-11-02 - v0.9.7 stable released on main

Maintainer-authorized promotion completed via [PR #8](https://github.com/DeadneM/Q-Protocol-007-First-Light/pull/8),
merge/source `8722000d859eb9785a8906cae82adc72d35ae1a2`.
PR Windows CI 41479969385 and main release CI
[41480287380](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/41480287380)
both SUCCESS: default-beta/preservation audits, updater transaction tests, native
DXGI smoke, embedded helper/default INI, ASI and actual CPU menu smoke all pass.

**v0.9.7 is published non-prerelease and verified as GitHub Latest.**
[Direct ZIP](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/download/v0.9.7/Q-Protocol_v0.9.7.zip),
584136 bytes, SHA-256
`9c7d30df6addd35d676f95566a2750606a4df233729d49a6b4a2fa5e192c8b4f`.
Downloaded and checked: CRC, exactly two AMD64 root binaries, all manifest hashes,
stable build identity and embedded default INI unchanged. README remains on GitHub.

Beta defaults OFF (including missing-key fallback); startup check ON. Explicit
existing preferences are retained. A28/A29 native gameplay and menu preserved.
The real updater engine finds v0.9.7 from A29 through stable and beta channels,
downloads/verifies it, and does not reoffer it to build 30. Results and probe
source are in `evidence/v0.9.7-verification.json` and `PublishedStableProbe.cs`.

Old v0.9.6/A28/A29 tags/assets remain unchanged. A29 remains installed locally;
no game launch or stable installation was performed during publication. Next:
use A29 Updates > Check now > Update on exit for the first actual exit-cycle
test. Keep the other system's GPU incident OPEN; it is not a universal fix.

## 2026-11-02 - v0.9.7 stable promotion authorized

Maintainer accepts A29 and explicitly requests beta updates OFF by default,
main integration and push/build release. Beta defaults are now false in native
state and the missing-INI-key fallback; startup checks remain ON. Existing
explicit preferences are preserved. Runtime updater/menu/native behavior is
otherwise identical to accepted A29, checked by `tools/verify_release_scope.py`.

Prepared stable v0.9.7 (updater build 30, newer than A29 build 29). The package
remains the two binaries only, plus a separate updater manifest release asset.
Release workflow is gated to main and refuses to overwrite an existing tag or
release. PR checks precede merge; main then builds/publishes its own source SHA.
Old v0.9.6/A28/A29 archives and personal installation are preserved.
Validation/publication pending. Real post-exit update cycle remains unconfirmed.

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

## 2026-11-02 - A28 accepted on maintainer system; A29 updater/menu TEST

Maintainer reports "ok parfait tout a lair de marcher" after installing A28.
Fresh logs confirm READY, complete F3 loadout, F1 transitions and F2 native ammo
publication for six classes. Evidence: `evidence/a28-runtime/`. A28 is the accepted
local base; the other system's GPU incident remains open.

User explicitly changes packaging: future ZIPs contain only `dxgi.dll` and
`QProtocol.asi`; README/default INI remain on GitHub. Existing releases are kept.
A29 embeds the updater helper and default INI, seeds a missing INI without
overwriting one, adds startup check/beta/explicit Update on exit and a readable
responsive menu. Reference: Darksiders Genesis Updates UI, inspected read-only.

Branch `dev/a29-updater-menu`, separate from main and A28. Local preservation
audit and 19 updater fixture checks PASS. Windows/native menu/package checks
pending. No A29 install or game launch. See [A29 notebook](A29_UPDATER_AND_MENU.md).

## A28 TEST ? 2026-11-01

Static remap prepared for Steam build 26252352; native code/type gate retained.
Windows CI 41357519256 SUCCESS; downloaded four-file ZIP, hashes and unchanged INI checked. Runtime validation pending. A26/A27 baseline preserved. GPU issue OPEN.
See [A28 notebook](A28_NOV2026_NATIVE_REMAP.md).

# Technical status

## First Debug — A21 first-open D3D12 device loss (2026-10-11)

- A user report contains successful A21 floating renderer initialization and first-frame submission, followed by `GetCompletedValue()==UINT64_MAX` (device removal), `GetDeviceRemovedReason()==0x887A002B` (`DXGI_ERROR_ACCESS_DENIED`), and a secondary `0x800705B4` frame-fence timeout.
- Latest release **v0.9.6 A25** still uses the A21 backend. Most urgent source gap: DIRECT queue inferred by same-device and thread-local observation, not positively linked to the swapchain's creation queue.
- Correct the log's misleading timeout/device removed labeling; capture DXGI creating queue; conduct DRED validation developer-side. Root cause unproven; **do not mark fixed** or require end-user experiments.
- Complete case record: [First Debug GPU Device Loss](FIRST_DEBUG_A25_GPU_DEVICE_LOSS.md). Status **OPEN**.


## v0.9.6 public release: A25 + native DXGI (2026-10-10)

- New public release v0.9.6 promoted from A25 per maintainer request, separate from historical v0.9.5.
- Floating ImGui menu defaults to 1000×850 constrained to current work area, with the Save / Reload / Defaults footer outside scrollable tab contents.
- Native x64 custom DXGI proxy is included in the four-file root ZIP and loads the matching QProtocol.asi alongside it. Back up existing local DXGI/ReShade/Special K wrappers before replacing.
- A24 gameplay, A23 adaptive 41-graph discovery, A22 mouse improvements, First Debug DX12 safety, weapon IDs and public QProtocol.ini are preserved.
- GitHub Actions audits C++ source isolation, Windows DXGI exported functions/ordinals, actual CreateDXGIFactory1 forwarding, overlay/API imports and ZIP root layout.
- v0.9.5 stays available for rollback; historical A23 crash root cause and wide GPU/Windows support are not proved by the build CI.
- Details: `packaging/RELEASE_NOTES_A25.md`. F1 close-combat research remains paused.

## v0.9.5 public release — A24

- User reports A24 appears to work in-game and requested publication.
- Runtime A23 log confirms graph index restored to 41 linked graphs, 42 ItemEntries, 43 spawners, with F3/AUTO and multiple weapon spawns completed.
- A21/A22 harden the overlay and mouse input; A23 repairs graph discovery around live player memory; A24 guards duplicate/burst requests (4 pending, 12 per rolling 30 seconds).
- F1/F2/F3/AUTO semantics, weapon IDs and `QProtocol.ini` defaults retained.
- Earlier A23 crash cause remains unproven, and cross-hardware compatibility still requires field feedback.
- `v0.9.4` is kept as the previous stable release. F1 close-combat research A20K–A20N remains paused.
- See `packaging/RELEASE_NOTES_A24.md`.

## A21 — archived overlay compatibility candidate (superseded by v0.9.5)

- F1 / close-combat research is paused.
- Active branch: `dev/a21-overlay-compat`.
- Gameplay/config base remains validated `v0.9.4 / A20J`.
- First Debug DX12 hardening is retained.
- Overlay is now a normal floating/movable/resizable ImGui window instead of a
  forced fixed-center panel.
- Input swallowing follows ImGui `WantCaptureMouse` /
  `WantCaptureKeyboard` / `WantTextInput` instead of consuming every input
  message while visible.
- Game cursor clipping is restored when the overlay closes/fails/shuts down.
- Run #131 / `37979873788`: **PASS**.
- Pre-release: `a21-test`.
- Status: **TEST / runtime validation pending**, not yet a public fix.
- Public `v0.9.4` remains untouched.
- Details: `A21_OVERLAY_COMPAT.md`.

## First Debug — public overlay compatibility investigation (2026-10-09)

- Report: users encounter a game crash on **Insert** (first display of the DX12 overlay); initial log is from A20I v0.9.3.
- Root cause unproven. Potential renderer first-frame initialization failure, swapchain mismatch, and unsafe DX12 resize/resource lifetime identified in source audit.
- Pinned ImGui and MinHook are built into the ASI; do not treat a missing end-user dependency as established.
- Fix on the development side, with safe fail-open overlay behavior. Preserve A20J gameplay functions, packaging and INI. **Do not require public users to run complicated tests.**
- Project notebook and developer acceptance criteria: **[First Debug](FIRST_DEBUG.md)**. Status: **OPEN**.

## Active state

- Current public release: **Q Protocol v0.9.6 / A25 Accessible Overlay + Native DXGI**.
- A20J is the validated post-update base for the October 7, 2026 executable.
- Player-ready resolution, F1, F2, F3, F4, F5-F12 and AUTO are validated in-game.
- Weapon spawning uses each RID's native ItemEntry/Spawner graph directly.
- F1 observes the game's native License To Kill state and forces the opposite state.
- Overlay tabs: Loadout / Weapons / Debug / Hotkeys.

- Behavioral reference: **U74** on the pre-update executable.
- Current post-update canonical build: **Fresh Core A20J**.
- Rejected compatibility branches: U80, U81, U81B, U82, U82A, U83, U84, U85.
- Gadgets: removed.
- Overlay/config model: locked.
- Source of truth: `/PROJECT_STATE.md`.
- **A20J Clean Core is validated in game and publicly released as v0.9.4**.
- A20J audit and validation are recorded in `docs/A20J_CLEAN_CORE_AUDIT.md`; no RID/gameplay/timing changes were introduced.

## Authoritative default controls

```text
F1  License To Kill
F2  Ammo +
F3  Manual weapon loadout
F4  Q-Pistol swap
F5  Weapon slot 1
F6  Weapon slot 2
F7  Weapon slot 3
F8  Weapon slot 4
F9  Weapon slot 5
F10 Weapon slot 6
F11 Weapon slot 7
F12 Weapon slot 8
```

Overlay toggle: **Insert by default, remappable from Hotkeys / `[Overlay] ToggleKey`**.

## Required core primitives

```text
ResolvePlayer()
ToggleLicenseToKill()
AddAmmo(player, profile)
ApplyWeaponLoadout(player, profile)
SwapQPistol(player)
GiveWeapon(player, weapon)
```

Manual and AUTO must call the same implementation.

## Manual / AUTO profiles

Both profiles are independently configurable.

Manual:
- `ManualLoadout.QPistol` chooses one Q-Pistol variant;
- `ManualLoadout.OneHanded` chooses one one-handed firearm;
- `ManualLoadout.TwoHanded` chooses one two-handed firearm;
- `ManualAmmo` chooses the ammo quantities F2 adds.

AUTO:
- `AutoLoadout` uses the same exact three typed roles;
- `AutoAmmo` chooses the ammo quantities AUTO adds.

The game loadout is never modeled as an arbitrary N-weapon list. F5-F12 are separate hotkeys, not loadout slots.

AUTO waits for the real player, then calls:

```text
ApplyWeaponLoadout(player, AutoLoadout)
AddAmmo(player, AutoAmmo)
```

When the player disappears or changes, AUTO resets.

Manual and AUTO use the same gameplay functions. No separate AUTO weapon/ammo engine is allowed.

## Overlay

Current runtime bindings are fixed by role:

```text
F1  LicenseToKill
F2  Manual Ammo
F3  ManualLoadout
F4  SwapQPistol
F5-F12 Weapon
```

F5-F12 weapon aliases are INI-backed. Generic F1-F12 `Action=` reassignment
is **not implemented yet** and the current `Action=` rows are descriptive.

The overlay toggle itself is remappable from the Hotkeys tab and persists to
`[Overlay] ToggleKey`.

Overlay controls include Save, Reload and Reset Defaults. INI remains the only
persisted configuration source.

## Primitive audit

Completed. See [`PRIMITIVE_AUDIT_OCT2026.md`](PRIMITIVE_AUDIT_OCT2026.md).

Key result: the October executable exposes a native `AddFirearmAmmunitionToPlayer` input, so F2/AUTO ammo no longer need the low-level reserve-vector setter.

The donor pair-clone path is retired. `GiveWeapon()` now resolves and triggers each weapon's native ItemEntry/Spawner graph directly.

## Next exact step

Fresh source implementation, in this order:

1. bootstrap/log/INI;
2. ResolvePlayer;
3. F1 LTK;
4. shared GiveWeapon queue;
5. F4 + F5-F12;
6. F3 ManualLoadout;
7. F2 ManualAmmo via native AddAmmo input;
8. AUTO via the same functions;
9. gameplay validation;
10. Insert overlay.


## Fresh Core A2 validation

Validated in-game on the October 2026 executable.

Confirmed working:
- clean ASI bootstrap;
- ResolvePlayer();
- F1 License To Kill;
- shared GiveWeapon();
- F4 Q-Pistol swap;
- F5-F12 configured weapon slots;
- minimal gameplay-thread native spawn hook.

A2 is now the canonical fresh-core base.

Next: corrected F3 ManualLoadout with exactly QPistol + OneHanded + TwoHanded, implemented only as a thin caller of the validated GiveWeapon queue.
