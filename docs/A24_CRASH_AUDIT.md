# A24: Weapon Burst Safety / A23 Crash Audit

Date: 2026-10-10
Branch: `dev/a24-weapon-queue-safety`
Status: **TEST candidate: crash root cause not yet proven**

## Source log

`QProtocol(4).log` provided after A23:

- The previously broken memory lookup is repaired: **41 linked graphs, 42 ItemEntry RID entries, 43 native spawners** on two PLAYER READY cycles.
- **Two AUTO packages** complete and publish ammo.
- **Two F3 MANUAL packages** complete every configured weapon role.
- **41 direct weapon requests** reach `DIRECT graph COMPLETE`, with native post-call `valid=yes accepted=yes` observations.
- **33 `queued RID` events** from repeated weapon hotkeys and other requests. Repeated F5–F8 hotkeys are visible at high frequency.
- **Zero log lines containing ERROR**. Log truncates after `F4 Q-Pistol` request reaches `DIRECT graph COMPLETE` after the second player READY cycle.
- This file does not record Windows exception code, crashing thread, exception address, driver/device-lost information, nor a stack. Do not infer a proven crash mechanism from the last successful log line.

## Immediate conclusions

A23 fixed the specific zero-graph regression of A22. Do **not** roll back A23's adaptive graph scanner as if it were still failing.

Rapid repeated weapon hotkey requests and cumulative native weapon creation are credible risks, **not proven causes**. Normal A20J queue logic accepts 16 pending requests, does not deduplicate RIDs, and permits rapid repeated enqueues with a 500-ms cooldown between completed spawns. A24 addresses this single bounded risk without touching native spawn internals or the previously validated gameplay features.

Other credible causes cannot be eliminated with this log alone: game entity lifecycle after level transition, a third-party overlay interacting with DX12, game engine resource exhaustion, or unrelated game crash.

## A24 minimal changes

Inside `src/QProtocol.cpp` only:

- `QueueWeapon`: ignore identical RIDs already pending or already actively being spawned.
- Limit queued distinct user-requested weapons to **4**.
- Limit newly accepted F4–F12 / Debug Spawn requests to **12 per rolling 30 seconds**.
- On READY player change / NOT READY, clear rate-limit history alongside normal queue reset.
- Emit `[A24]` logs identifying suppressed duplicates, overflow and burst limit.
- Do not change AUTO, F3 Manual Loadout, F1 LTK, F2 Ammo, RID catalog, NativeSpawn, `QpGameplayTick`, graph search, 500 ms pacing, ImGui DX12 backend or overlay.
- User-selected INI values and default `QProtocol.ini` stay unchanged.

## CI

- Retain A23 graph scanner and A22 input changes.
- Assert `GameplayHook.asm`, `Overlay.h`, and `QProtocol.ini` exactly match public stable v0.9.4.
- Source isolation check against A23 for `QProtocol.cpp`; changes permitted **only** in `QueueWeapon`, its guard constants/history and reset of that history.
- Flat ZIP with `QProtocol.asi`, `QProtocol.ini`, `README.txt` at root.

## Acceptance

1. Game no longer terminates after repeated weapon hotkeys, including level transitions.
2. Log records the A23 graph index (41 or equivalent) and `[A24]` suppressions instead of unbounded spawn requests during a burst.
3. F1, F2, F3, F4–F12, AUTO and Debug Spawn remain operational for ordinary use.
4. Insert, mouse controls, title bar, Alt-Tab remain operational.
5. If the crash persists, treat the weapon-burst hypothesis as unconfirmed and do not promote this candidate.

Public v0.9.4 and test A21–A23 releases remain untouched; research A20K–A20N F1 close-combat remains paused.
