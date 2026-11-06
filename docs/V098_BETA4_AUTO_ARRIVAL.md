# v0.9.8-beta.4 — first AUTO item completion

Base: beta.3 source `2f0ef70`, with the maintainer's accepted instant manual-delivery checkpoint recorded at `a06db56`. Branch `beta/v0.9.8-auto-arrival`, internal build 42. Stable behavior reference remains v0.9.7; main is not promoted.

Objective: receive the configured AUTO weapons on the first Tac Sim lobby arrival without needing F3. Success requires actual receipt of all three configured weapons after a fresh launch, followed by preserved instant F3/F4 behavior. AUTO death/reload and pause/menu lifecycle remain separate acceptance checks; this change does not rewrite that state machine.

## Evidence and scope

The maintainer launched beta.3 into the Tac Sim lobby without F3/F4 and reports AUTO absent with Bond controllable. A bounded read-only capture saw all three created outputs become generation-invalid, with no addition to Bond's four-item inventory. For two weapons, a valid runtime item ID became zero before the queued pickup could run. This is item loss, not just a missing confirmation. Raw captures stay private; [sanitized evidence](../evidence/nov2026/beta4-auto-arrival.json).

All three AUTO sources contain index 0 in their native +F0 index vector and retain one created job after this failure. Native finalization at `0x016B7450` tests the current created-job count against this vector. A match takes the +1B0 virtual branch, which publishes event 0x19D with item ID and throw position/direction; the ordinary +188 path completes placement. The same silenced RID succeeds later after its source count advances past index 0. The manual Taser/shotgun sources have empty +F0 vectors and succeed on their first creation. This supports isolating the source's first-index world-throw policy from an inventory grant; it does not prove every possible item deletion has this cause.

The requested first F3 completed before the replacement observer attached; its log, post-action inventory and the maintainer confirm all three receipts. A later captured F3 used a changed manual profile and was followed by F4. Those observations are not presented as a single controlled manual timing experiment. No further player experiment is requested to justify another timing guess.

## Change

Only explicit AUTO spawn invocations register a five-second bounded ownership record. At the native special-index completion call site, the new read-only observer requires a fresh matching player/session/epoch, live source generation and matching RID, valid created IItem/registry identity, readable inventory and an exact output in that invocation's still-live logical batch. Unknown, expired, ambiguous or historical records leave the native operation unchanged.

An owned AUTO item selects the same native +188 placement method used by the ordinary completion branch, in place of that item's world throw. The hook jumps back to the original native CALL instruction; the native return/unwind site and subsequent finalization remain in the game. No source object, vtable, policy vector or weapon ID is modified. Ownership is also acknowledged while the logical batch is live, so the worker can keep the assigned job/output after the native same-frame clear without adopting historical capacity.

Pickup itself is the unchanged beta.3 native mode 2, with unchanged item/player/attachment revalidation, receipt confirmation, safety limits and no replay after an uncertain native operation. F3/F4 requests are not registered in this AUTO ledger. No arbitrary startup delay, sacrificial weapon or repeated AUTO package is added.

## Preservation and risk

Graphics/startup/menu, existing gameplay hooks, player/session/AUTO state machine, catalogs, INI/defaults, updater, ammo primitive and native spawn/pickup implementations remain source-identical to beta.3. Additional preimage and vtable-target guards cover the new hook and both native methods. A hook installation failure leaves AUTO closed. Personal files and the installed beta.3 pair are untouched while preparing this candidate.

The new native routing is **EXPERIMENTAL**. Static checks and the x64 CPU routing harness can check ownership rejection, argument/return preservation and correct native-method selection, but cannot establish actual in-game delivery, every game-world throw or GPU stability. The existing beta.3 installation and verified rollback remain available. Installation requires the game closed and preserves the current personal INI, including any edits made during this session.

## Checks and acceptance

- Local source-preservation and exact supported-EXE guard checks.
- Owned-completion regression cases: stale/wrong/expired tickets, source/RID/generation change, job reuse, historical capacity, reset and ambiguous requests.
- Actual assembled x64 hook CPU test: unowned operation uses original method exactly once; owned operation uses placement exactly once; arguments, original return site, return value and source vtable preserved.
- Full Windows build plus existing native readers, lifecycle/mailbox, updater, real DXGI forwarding/dependency, menu/bootstrap and two-binary ZIP checks.
- Maintainer: fresh launch to Tac Sim lobby, no F3 until AUTO receipt is assessed; verify all three actual weapons, then F3/F4 remain instant. Story and death/reload follow only after that checkpoint succeeds.

Status **BUILT / CHECKED / EXPERIMENTAL**, gameplay acceptance pending. Frozen source `2fd0668e40308e124aaf829b138412ec57c8fb08`; [full Windows CI](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/42215454049) successful. The first run stopped on strict MSVC fixture-name shadowing, corrected in the test only. The successful run passes 21 owned-completion checks, the actual assembled hook CPU smoke (11 checks), 43 inventory contracts, 56 session/mailbox checks and the existing updater/menu/DXGI/package checks. Local supported-EXE verification covers 32 code ranges and eight pointer links.

Install ZIP `Q-Protocol_v0.9.8-beta.4.zip`: 605230 bytes, SHA-256 `1bfc12285209c0b088153a40491a89f9f970f5aabd75f76a8d56d2409e9d99d1`. Downloaded from successful artifact `13501001819`, with ZIP CRC, manifest sizes/hashes and both AMD64 binaries verified. Root contains only `dxgi.dll` and `QProtocol.asi`; documentation remains on GitHub.

Beta.3 remains installed. No game launch or installation of beta.4 by the agent. Publication and actual updater discovery checks follow the frozen artifact; no rebuild during publication.
