# U67 delta audit — current test build vs canonical U57

## Status

- **Canonical stable lifecycle base:** U57
- **Current integration/test base:** U67
- **U67 user result:** core systems work again; manual F2 executes; AUTO/lifecycle execute; gadget identities are still wrong / not trustworthy.
- **Canonical promotion:** deferred until the seven gadget identities are re-established from authoritative runtime objects.

## Exact package hashes

### U57 canonical lifecycle base
- `QProtocol.asi` size: 79,360 bytes
- SHA-256: `e0e3cf2df49d03b422fbdfcb868d3ca107619b26d004078d5b3f3eb65f31f192`

### U60 identity-mapping base
- `QProtocol.asi` size: 79,360 bytes
- SHA-256: `a35dacbca68b0b02b1b266b8d52e7e35718753fa4c511f1e328ff2d7a8300eaa`

### U67 current test build
- `QProtocol.asi` size: 82,944 bytes
- SHA-256: `8c6859628d64641fb0c57010f782b35307fa425364b22cc46e0aa70b9b7d07cb`
- ZIP SHA-256: `66def588819f76926030a4ae46e60cd049d803003ec612d91b67c7d15309eec1`

The packaged `QProtocol.ini` is byte-identical in U57, U60 and U67:

`b354812f3d32cf5bdba349976f0eb86d109ce9b2f6023fa4c835fe00fcfba207`

## Binary delta size

Compared with U57:
- 324 differing bytes in the overlapping 79,360-byte region;
- 3,584 additional file bytes from the scanner section;
- total positional delta: 3,908 bytes.

Compared with U60:
- 53 differing bytes in the overlapping region outside the appended scanner payload;
- 3,584 additional file bytes;
- total positional delta: 3,637 bytes.

Most of U67 is therefore still U60/U57. The large apparent change is the appended scanner infrastructure, not a rewrite of the existing gameplay engine.

## What changed from U57 to U60

U59/U60 changed gadget token learning/mapping while preserving the validated U57 AUTO lifecycle.

U59 added identity-based runtime-token learning for four target gadgets using `selectedObject+0x108` fingerprints:

- SmokePellets: `AED6146A0137DAD1`
- MissilePen: `7535800D013167C9`
- Quick Hack: `A04C60F401FBB751`
- Dartgun: `55DDE49601591F97`

U60 changed only the four-byte target-record table so the real record order is used:

- record 0 = Left
- record 1 = Up
- record 2 = Down
- record 3 = Right

Desired target mapping remains:

- Left = MissilePen
- Up = Dartgun
- Down = SmokePellets
- Right = Quick Hack

U57's safe AUTO gadget latch and level-transition protections remain the lifecycle foundation.

## What U67 adds on top of U60

### PE layout

U60:
- 7 sections
- `SizeOfImage = 0x219000`
- `.q31d` VirtualSize `0x60`

U67:
- 9 sections
- `SizeOfImage = 0x22B000`
- `.q31d` VirtualSize `0xA0`
- new `.q61` at RVA `0x219000`, RX, raw size `0xE00`
- new `.q61d` at RVA `0x21B000`, RW, VirtualSize `0x10000`, no raw data

`.q61d` is the dedicated 64-KiB scan buffer.

### Runtime-player anchor hook

At RVA `0x5BB9`, U60 executes:

```asm
mov rbp,[rsp+0x78]
mov rdx,rbp
```

U67 routes those instructions through a helper that also captures the exact runtime-player-ready object as the scan-generation anchor.

### Automatic one-shot gadget scan

The existing U57/U60 ready path at RVA `0x216385` is routed through the U67 poller.

The poller:
- starts one background scan once a valid player anchor exists;
- does not require B;
- does not run continuously;
- uses the U61 read-only `VirtualQuery` + `ReadProcessMemory` scanner;
- scans at most ±512 MiB around the runtime-player object;
- uses a dedicated 64-KiB buffer.

### AUTO-only readiness gate

At RVA `0x216350`, only the U57 AUTO gadget pending path is gated:

- list ready → continue through the original U57 safe F2 synthesis;
- scan still running → wait and leave the pending latch set;
- scan finished but incomplete → fall back to original U60/U57 behavior.

**Manual F2 is not gated by U67.**

### Transition cleanup

The U57 player-not-ready reset tail at RVA `0x2163B6` is routed through a U67 cleanup helper which clears the scan generation, cached tokens and the published U60 identity-token state before resuming the existing lifecycle reset.

### U60 remapper itself

U67 restores the exact U60 fingerprint-load instruction at RVA `0x217300`.

Therefore the `.q31` identity-remapper code is byte-identical between U60 and U67.

The major U67 behavioral change is that the scanner pre-populates the same U60 identity-token table before AUTO/manual F2 consume it.

## Current U67 log result

```text
U67 GadgetList loaded mask = 0x75
U67 GadgetList target mask = 0x0F
U67 GadgetList READY = 1
U67 Dartgun token = 0x0A010000
U67 SmokePellets token = 0x0A010001
U67 MissilePen token = 0x0A010008
U67 QuickHack token = 0x0A010002
```

`0x75` means the scanner classified Dartgun, Laser, SmokePellets, MissilePen and Quick Hack, but not BlastDevice or ShockWave.

`0x0F` means U67 believes all four target runtime tokens were found.

The same log proves manual F2 still reaches the normal native path:

```text
F2 native gadgets requested.
F2 native gadgets + Q-Lens applied directly.
```

However the actual wheel is wrong. The remaining defect is therefore the identity data being published into the U60 table, not F2 execution.

## Validation decision

- U57 stays the canonical stable lifecycle fallback.
- U67 is accepted as the current working integration/test base.
- U62–U66 remain rejected.
- U67 is **not** promoted as the final canonical gadget build until the seven gadget identities are recaptured authoritatively.
