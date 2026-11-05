# v0.9.8-beta.1 — owned weapon delivery

The accepted stable reference is v0.9.7. This candidate repairs delivery on Steam build 26430236, retaining the A38 executable map and experimental local-character/session gate. Main remains on the accepted stable line. Internal updater build 39 is displayed separately from version v0.9.8-beta.1.

## Reproduced failure and correction

The maintainer prepared a controllable Tac Sim lobby on A38. The bounded read-only inventory snapshot contained four gadgets. One F3 was then observed without injecting or triggering an action. The maintainer received the Taser and shotgun, with the silenced Q-Pistol missing. The next inventory snapshot showed two additional firearms. The separately created silenced pistol had a live generation, a valid IItem interface and a registered item ID, absent from Bond's attachment list.

The observed creation record begins with a provisional loading-job handle of -1. It changes to an assigned handle before output appears. The output is briefly in the logical vector, which the game then clears. A36 incorrectly required the initial handle to remain immutable and rejected output after the clear. Its observer therefore failed before the proposed native pickup could be dispatched.

The repair captures a record immediately after its own native spawn call. It learns the assigned job while that record is in the logical batch. A completed record may be read after clear only if the same job, source generation and allocation are still associated with this request. A new or unknown job cannot adopt historical capacity. Actual item identity is checked against the native IItem and runtime ID registry.

For an owned live item absent from the current player's attachment list, publish one native pickup event using the game's lock, insertion and notification path. The gameplay callback rechecks player, item generation, item ID and membership immediately before publication. An already attached item is skipped. Unknown state, cancelled sessions and expired generations cannot authorize pickup.

The original accepted-spawn-to-idle queue progression and 500 ms cadence continue independently of receipt observation. Later roles can proceed. AUTO ammo runs after the parallel delivery observations finish, with a five-second bound from each native call, even if a result remains unconfirmed. Native spawns and pickups are not blindly repeated. Existing session/menu/death policy and its limitations are retained.

Graph selection also records the actual source entry linked to the selected spawner when a RID has multiple entries, keeping the request and observed native source paired.

## Acceptance

Status **EXPERIMENTAL** until the maintainer confirms actual receipt. First test F3 once in the same controllable Tac Sim lobby with the mod menu closed. All three configured weapons must arrive, including the silenced Q-Pistol. Check the actual inventory and compare the log's receipt confirmations. Then test AUTO on arrival, ammo, menu/pause without repeats, a Tac Sim mission and story/death reload. Same-identity reload without observed death remains open.

Read-contract regression tests cover provisional-to-assigned jobs, clear after an observed assigned job, unknown historical capacity, changed job/source and stale object generations. Compilation, guards, synthetic fixtures and updater discovery are technical checks. The new pickup path remains a gameplay experiment until tested.

## Build verification

Frozen source `96cd47002b18ec07dd418550fc71b4e0526b3457`, [successful full CI run](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/42064315865). Local checks verified the two root x64 PE binaries and ZIP integrity. ZIP 599997 bytes, SHA-256 `ad5db48b3565474a0f845cf3fa6656b025a39ff5b1646e4d05fc4dce84ee0650`.

Status **BUILT / CHECKED / EXPERIMENTAL**. Gameplay acceptance pending the single F3 inventory test.
