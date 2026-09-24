# U49 patch record

## Status

U49 is a proof build reconstructed strictly from the canonical U33 ASI.

- U33 SHA-256: `a0ac354d7c7f2c96ceae2fa3da6cadc0756d325d9fb796a4da2083a223f10dce`
- U49 SHA-256: `ba119fd8b781b15c1bc41bf393cddcd5dc7a3836ab9081426e4e6a01245a3a03`
- File size: unchanged, 79,360 bytes.
- Total changed bytes versus U33: 26.

## Functional goal

The user-directed rule is simple:

> If F2 works, F4 works, and AUTO weapons work, then AUTO gadgets should be triggered at the already-validated AUTO weapon success point by calling the existing F2 path.

No separate gadget readiness detector is introduced.

## Exact binary diff

### 1. AUTO weapon success hook

- File offset: `0x00007BD4`
- RVA: `0x000087D4`

Before:

```text
48 8B 05 FD BB 00 00
```

After:

```text
E9 37 DB 20 00 90 90
```

This jumps to the U49 trampoline at RVA `0x216310`.

### 2. Embedded build banner

- File offset: `0x000101F0`

Before:

```text
33 33
```

After:

```text
34 39
```

This changes the embedded U33 label to U49.

### 3. Trampoline

- File offset: `0x00012F10`
- RVA: `0x00216310`
- Location: unused executable cave in the existing `.q25` section.

Before:

```text
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```

After:

```text
E8 EB FC FF FF
48 8B 05 BC E0 DF FF
E9 BA 24 DF FF
```

Semantics:

1. call the exact MANUAL F2 wrapper at RVA `0x216000`;
2. replay the displaced original MOV;
3. return to RVA `0x87DB`.

## Intentionally unchanged

- MANUAL F2 call site at RVA `0x7967`.
- F2 wrapper `.q25` at RVA `0x216000`.
- Native four-record spawner at RVA `0x216200`.
- U30/U31 gadget remapper.
- Retail producer hook.
- Positional runtime-token mapping.
- All U44–U48 experiments.

## Validation

The first U49 gameplay test reported that it **appears to work**. It is not promoted over U33 as canonical until broader level and respawn testing is complete.

## Rebuild

Use:

```bash
python tools/build_u49.py /path/to/U33/QProtocol.asi -o QProtocol.asi
```

The script verifies both the U33 preimage SHA-256 and the exact U49 output SHA-256.
