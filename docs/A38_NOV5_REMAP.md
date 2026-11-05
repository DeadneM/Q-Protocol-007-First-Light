# A38 November 5 Remap TEST

Adaptation to Steam build **26430236**, using the saved executable from build 26252352 for comparison. The accepted behavior reference remains v0.9.7. A38 carries the experimental A37 queue and A35 player/session handling. Those changes still require gameplay acceptance.

The supported EXE SHA-256 is `f92c0e8600aa3b692a437f2c0795068f509db1a73a4794b619e6d60bd5d35c43`, timestamp `0x6AEAFDDA`, image size `0x06D32000`. [Recorded map and instruction comparisons](../evidence/nov2026/a38-native-map.json).

## Changes and checks

- Remapped spawn, ammo insertion/notification, LTK decision/observer and gameplay callback sites. The callback epilogue and LTK observer instructions match the existing assembly replay.
- Located the native local-character binding, generation table, player registry, runtime root and type tables individually. RTTI names and base offsets confirm the local-character getter, player spawner, loadout, item entry and gameplay spawner types. A uniform address shift would be incorrect.
- Compared instruction spans for the native primitives and read contracts. Registers, scalar offsets and calling sequences match outside the enclosing gameplay function. That function reorders six member reads while keeping the hooked epilogue, stack restoration and instruction layout. A38 does not read those six members.
- Refreshed all 18 native guard byte ranges and verified all six relocated pointer links against the saved new EXE. Both hook preimages match. PE checks and exact guard rejection remain active.
- Re-read the updated RPKG indexes. All 31 configured weapon identities remain present as TEMP, with the expected path hashes and no indexed patch deletions for those IDs. This verifies identities. The silenced Q-Pistol's actual delivery remains open.
- The scope check compares A38 to frozen A37 source `ea5073c`, allowing only the reviewed address/guard changes and build identity in the runtime code and fixtures.

Reproduce the offline checks from the repository with `python tools/verify_a38_scope.py` and `python tools/verify_a38_executable.py <new-exe-path>`. The executable and personal snapshots are kept privately outside Git.

## Test and status

Goal for this build is to load on the updated game with the native compatibility guards passing and execute the existing manual/AUTO path. Check recognition in the lobby, F3 with the menu closed, ammo and LTK, then enter a Tac Sim mission and check AUTO once on entry. Also check a story mission and death/reload. Use the [A37 checklist](A37_WEAPON_CADENCE.md) to track the previous regression.

Status **EXPERIMENTAL**, gameplay validation pending. Packaging is two root binaries, `dxgi.dll` and `QProtocol.asi`. Personal INI and bindings are retained. Stable promotion requires explicit acceptance.

## Build and installation

Frozen source `5bc3ac2d64567328b6ab079bb79599ba890cf5ec`. [Full CI run](https://github.com/DeadneM/Q-Protocol-007-First-Light/actions/runs/42022238890) succeeded. ZIP verified locally, 591373 bytes, SHA-256 `7bc3cd58d903e2bd2fa9f1052119f8d89a7798774b7554bc9e240c16c7944150`.

Installed on 5 November with game/updater closed, a verified private backup of the previous binary pair, and identical personal INI hashes before/after. Installed ASI/DXGI hashes match the checked artifact. No game launch or gameplay validation by the agent. Status **BUILT / CHECKED / INSTALLED / EXPERIMENTAL**.

[A38 TEST release](https://github.com/DeadneM/Q-Protocol-007-First-Light/releases/tag/a38-nov5-remap-test) published from the frozen source; publication run `42022783986` successful. Public ZIP hash and tag verified. The unchanged updater engine, using the new game EXE, discovers A38 with beta enabled, downloads/verifies its ZIP, excludes it from stable discovery and does not offer it again to build 38.
