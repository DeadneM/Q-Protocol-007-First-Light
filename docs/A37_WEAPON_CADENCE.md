# A37 Weapon Cadence TEST

The test targets the waits and loss of AUTO ammo observed in the A36 Tac Sim lobby log. The behavior reference is v0.9.7 (`8722000`). Source starts before the rejected delivery gate at `66c91ec` and carries forward the experimental A35 local-character/session observer, which was never independently accepted in gameplay.

- When an accepted native request returns to idle, the worker advances the same weapon queue used by F3, F4, the catalog and AUTO, with the original 500 ms cooldown. This describes spawner processing. Actual receipt must be checked in the game.
- If the native call was entered and its outcome is uncertain, record a warning, advance to later roles and allow the existing same-session AUTO ammo phase. The role is retired without replay. Pre-call resolution retries and the timeout for a genuinely busy spawner still apply.
- The compatibility gates, runtime IDs, RIDs, input bindings, menu, updater and INI defaults are carried forward. The A36 attachment gate and additional pickup event are outside this build.

## Gameplay acceptance

1. In the Tac Sim lobby, let AUTO run with the existing profile. Check the weapons and ammo counters, then press F3 once with the menu closed. The previously working weapons should arrive without successive five-second attachment waits.
2. In a Tac Sim mission, check AUTO on entry, open/close the mod menu, and check that AUTO applies only once for that playable session. Repeat F3 and note separately whether the silenced Q-Pistol arrives.
3. Check a story mission and a death/reload. A reload with the same character identity and no observed death remains unresolved. Use the log to distinguish dispatch, spawner idle and uncertain outcomes.

The silenced Q-Pistol's delivery remains open. The audited RID identifies the correct asset. The next investigation should correlate a created object with its own request and establish which native step attaches it to Bond.

## Verified build

Source `ea5073cd83c9506c6d2f73e058da06d6f7de889a`, [full CI run](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/41967123841) successful. The ZIP contains only `dxgi.dll` and `QProtocol.asi` at the root. Both binaries passed local x64 PE, integrity and hash checks.

ZIP SHA-256 `1662918707abeeaa1c1c864014040eace7f4e6c67139b041c1a1bfe2c6013a9a` (591361 bytes). Status **BUILT / CHECKED / EXPERIMENTAL**, pending the gameplay checklist above.
