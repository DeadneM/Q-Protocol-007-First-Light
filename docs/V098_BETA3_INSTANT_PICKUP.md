# v0.9.8-beta.3 — instant scripted pickup and confirmed F4

Base is beta.2 source `c12ee2d` for the reported-working startup/menu path, with the accepted stable behavior reference v0.9.7. The maintainer reports no further startup crash on beta.2, but intermittent silenced Q-Pistol receipt. This is partial acceptance of the startup correction, not global validation of the mod.

## Evidence and change

The beta.2 log confirms all three F3 inventory receipts in one package. Repeated F4 alternates normal/silenced resource IDs. Normal variants consistently receive attachment confirmation, while several silenced variants publish the native pickup but expire without inventory confirmation. Initial AUTO also records pre-call pickup failures. Raw logs and personal configuration remain private.

The native `ZCLTriggerPlayerItemPickup` action maps member `m_skipPickupAnimations` at +0x28 to mode 2. [Executable metadata and branch evidence](../evidence/nov2026/beta3-instant-pickup.json) records the supported EXE, reflected member offset and exact native branch bytes. Use this scripted instant-pickup mode for the mod's owned created items. The existing live item, current player and attachment revalidations remain active. This removes the normal pickup animation from scripted grant timing; gameplay effectiveness remains to be checked.

F4 holds one pending variant until its own bound request is confirmed in the player's inventory. Enqueue and spawner completion do not flip alternation. Failure/cancellation clears the pending request while keeping the desired next variant. Older or duplicate acknowledgements cannot flip it. Other weapon requests retain their existing queue and safety limits.

Pickup replies now distinguish player change, missing TLS, changed item, unreadable inventory, missing native owner, already attached, published and native exception. Only failures returned before any native mutation can retry revalidation/publication, with 100 ms spacing and the existing five-second observation deadline. Published or possibly partial native operations are never replayed.

The log's final beta.2 burst also reaches the existing 12-unitary-requests/30-seconds guard, which then suppresses other weapon hotkeys temporarily. That safety limit remains in place. Acceptance should use a controlled set of inputs.

## Acceptance

Status **EXPERIMENTAL**. Keep the beta.2 startup and menu correction intact.

1. Enter the Tac Sim lobby and press F3 once with the mod menu closed. Check actual receipt of the configured three weapons.
2. Press F4 six times at roughly one-second intervals. With the existing INI, the first variant is normal, then silenced, alternating after each confirmed receipt. Each requested variant must actually arrive.
3. Check AUTO on a fresh mission arrival and inspect any detailed pre-call failure instead of inferring receipt from a published event.

Scope checks preserve the complete beta.2 overlay/backend, native map, character/session readers, creation tracking, ammo primitive, config and updater. Targeted regressions exercise pre-call retry eligibility, no replay after publication/fault, late/duplicate F4 receipts, failure retaining selection and session cancellation.

## Verified build and installation

Frozen source `2f0ef7054268fda81bb31c70839899f8e42897dd`, [full CI run](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/42121467744) successful. Local ZIP integrity, x64 PE and per-file hash checks passed. ZIP 601039 bytes, SHA-256 `5d91a9390878220a686f64ce6f81fd898c93655cf09a154569d5009ba6cd51b1`.

Installed game/updater closed, with verified backup of beta.2, exact artifact hashes and identical personal INI before/after. No game launch by the agent. Status **BUILT / CHECKED / INSTALLED / EXPERIMENTAL**, acceptance pending F3/F4 tests.
