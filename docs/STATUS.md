# Technical status

## Current branch state

- **Canonical fallback:** U33.
- **Current proof build:** U49.
- **U49 first user result:** appears to work.
- **Promotion status:** not yet canonical; needs broader level/respawn validation.

## Validated gadget lineage

### U30
Validated four-gadget runtime-token remapper. In the known-good level the desired mapping is:

- Up = Dartgun
- Right = Quick Hack
- Down = SmokePellets
- Left = MissilePen

### U31
Integrated the U30 remapper into Q Protocol.

### U33
Current canonical fallback. Manual F2 works in known-good levels. Legacy AUTO gadget writer is disabled in configuration.

### U39
Diagnostic milestone. Runtime gadget identities can be resolved independently of vanilla slot position. U39 also proved that a non-equipped MissilePen can still have a valid runtime token.

## Rejected AUTO gadget branches

U44–U48 are documented as rejected experiments and must not be used as foundations.

The main lesson from those builds is that gadget AUTO should not invent a second readiness system or a second writer while the weapon AUTO path already has a working runtime-ready moment.

## U49 architecture

U49 is rebuilt strictly from U33.

The existing weapon helper is shared by MANUAL F4 and AUTO. U49 hooks only the AUTO success path. When the AUTO weapon package has been accepted successfully, U49 calls the exact existing MANUAL F2 wrapper from the same gameplay frame.

```text
AUTO weapon helper success
        |
        v
exact F2 wrapper (.q25)
        |
        +--> System 13: Q-Lens
        +--> System 14: GadgetActivation
        |
        v
native 4-record gadget spawner
        |
        v
U30/U31 remapper
        |
        v
retail producer
```

### Frozen U33 paths preserved by U49

- MANUAL F2 call site: RVA 0x00007967
- exact MANUAL F2 wrapper: RVA 0x00216000
- native four-record spawner: RVA 0x00216200
- U30/U31 remapper: unchanged
- retail producer hook: unchanged

### U49 patch

The AUTO weapon path uses the same shared weapon-package helper as F4.

- MANUAL F4: helper mode 1
- AUTO: helper mode 2
- only the AUTO success path reaches the U49 patch point

U49 replaces the 7-byte instruction at RVA 0x000087D4 with a jump to a small trampoline in pre-existing executable cave space at RVA 0x00216310.

The trampoline:

1. calls the exact MANUAL F2 wrapper at RVA 0x00216000;
2. replays the displaced original MOV;
3. jumps back to RVA 0x000087DB.

No new PE section, no new producer hook, no alternate gadget writer, and no extra timer/readiness detector are introduced.

## Current known limitation

The U30/U31 remapper learns tokens by **vanilla slot position**, not by gadget identity. This works in the known-good level but can map the wrong gadgets in levels with a different vanilla quartet.

Identity-based correction is a later stage. Do not merge the rejected U40–U43 resolver/hook experiments back into the functional base.

## Next validation

1. Confirm U49 AUTO gadgets across more than one level.
2. Confirm respawn / player replacement.
3. Confirm MANUAL F2 remains unchanged after AUTO.
4. Only after U49 survives those tests, separate MANUAL/AUTO gadget profiles.
5. Then revisit cross-level identity using the U39 findings without replacing the validated U30/U31 producer path.
