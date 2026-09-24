# Development history and current findings

This file is a compact reconstruction of the important Q Protocol gadget/weapon lineage. Rejected branches are retained so they are not accidentally reintroduced.

## Weapon architecture

The validated weapon path uses game-native spawning. Manual F4 and AUTO share the same runtime/player readiness infrastructure, while F5–F12 use validated pair-clone firearm spawning.

Important validated Q-Pistol RIDs:

- Silenced Q-Pistol: `019973508A5E327B`
- Unsilenced Q-Pistol: `01BE2C45AA55200D`

Validated pair-clone probes include:

- LightPistolNonLethal: `016886A4B599391C`
- Taser: `01702FE09FEA3596`
- ARExotic: `01587BEED983569A`

The weapon AUTO path is important to U49 because it provides an already-proven point where the current gameplay frame is ready to deliver equipment.

## Gadget producer and record format

The native gadget route reaches the retail producer at:

- EXE + `0x1343F80`
- observed retail caller return address: `0x1416C8B0A`

Validated 24-byte record layout:

```text
+0x00 dword player/context id
+0x04 dword numeric slot
+0x08 qword 0xFFFFFFFFFFFFFFFF
+0x10 dword runtime item token 0x0A01000X
+0x14 dword state, commonly 1 or 2
```

The gadget identity used by the producer is the runtime token at `+0x10`, not the qword at `+0x08`.

## Direction mapping

Validated numeric slot directions:

```text
0 = Left
1 = Up
2 = Down
3 = Right
```

Desired quartet:

```text
Up    = Dartgun
Right = Quick Hack
Down  = SmokePellets
Left  = MissilePen
```

## U30

U30 validated the four-slot remap in game.

The U30/U31 remapper learns live tokens from vanilla retail assignments, then remaps Q Protocol's placeholder records. This works in levels where the vanilla quartet lines up with the positional learning assumptions.

## U31 / U33

U31 integrated the U30 remapper into Q Protocol.

U33 is the canonical fallback. Its ASI is functionally the U31 gadget engine with AUTO gadget disabled through the INI so the old direct-writer path does not compete with the validated manual path.

Canonical U33 ASI SHA-256:

`a0ac354d7c7f2c96ceae2fa3da6cadc0756d325d9fb796a4da2083a223f10dce`

## Cross-level limitation

The U30/U31 remapper learns by vanilla **slot position**, not by gadget identity. In a different level, vanilla Left may be ShockWave instead of MissilePen, so the same positional assumptions can produce the wrong gadget identities.

## U35–U39 diagnostics

These diagnostics isolated the identity problem.

U39 proved that desired gadgets can be resolved by stable runtime fingerprints and that MissilePen can have a valid runtime token even when it is not one of the four currently equipped vanilla gadgets.

Stable instance RIDs:

- MissilePen: `01400A15903C9985`
- SmokePellets: `01BA24E28342EA32`
- Quick Hack: `019CF34A2C59C76F`
- Dartgun: `01C315FC8C1AEF95`

U39 is a research milestone, not a gameplay base.

## Rejected U40–U43 branch

U40–U43 attempted to merge identity resolution into new producer hooks / bridges. These builds either crashed, suppressed output, or added too many moving parts. They must not be used as foundations.

## Rejected U44–U45 branch

These attempted to introduce Manual/Auto gadget profiles with an extra wrapper layer. The wrapper diverged from the validated U33 path, and the first implementation also mishandled internal RID representation. Rejected.

## Rejected U46–U48 branch

U46 called the F2 wrapper from a separate AUTO point but could run before the U30/U31 remapper had learned its live tokens.

U47 retried based on the wrong readiness signal and still mixed the legacy direct writer with the native producer.

U48 introduced another dedicated readiness/helper chain. It still did not solve AUTO and violated the simplification goal.

## U49

U49 follows the user-directed simplification:

```text
AUTO weapons succeed
        ↓
call exact existing F2 wrapper
```

The patch is applied only on the successful AUTO weapon path, inside the same gameplay frame used by the functioning manual systems.

No new gadget writer, producer hook, timer, or independent readiness detector is added.

First gameplay feedback: **appears to work**.

## Next work

1. Validate U49 across multiple levels and respawn/player replacement.
2. Confirm manual F2 remains unchanged.
3. Promote U49 only after those tests.
4. Then separate MANUAL/AUTO gadget configuration if still desired.
5. Finally address cross-level gadget identity using U39 findings, while preserving the validated U30/U31 producer path.
