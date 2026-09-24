# Q Protocol v0.8.12U49 — Test Build

**Status:** prerelease / test build  
**Canonical fallback:** U33

U49 is rebuilt strictly from the validated U33 base. It does not reuse the rejected U44–U48 gadget-auto experiments.

## Main change

After the validated automatic weapon package succeeds, U49 calls the exact existing manual **F2** gadget wrapper from the same gameplay frame.

```text
AUTO weapon package succeeds
        ↓
exact MANUAL F2 wrapper
        ↓
Q-Lens + GadgetActivation
        ↓
native four-record gadget spawner
        ↓
U30/U31 runtime-token remapper
```

The legacy `AutoSpawnGadget` direct writer remains disabled.

## Validation state

Initial in-game testing is positive, but U49 remains a prerelease until broader level, respawn, checkpoint and transition testing is complete.

## Release ZIP

The downloadable ZIP intentionally contains **exactly three files at the archive root**, with no parent folder and no extra metadata files:

- `README.txt`
- `QProtocol.asi`
- `QProtocol.ini`

## File checksums

```text
094d07139fa858f34955bc32812a7d90c60bf1a27a32b4d74e3b5ae7c144fc91  README.txt
ba119fd8b781b15c1bc41bf393cddcd5dc7a3836ab9081426e4e6a01245a3a03  QProtocol.asi
b354812f3d32cf5bdba349976f0eb86d109ce9b2f6023fa4c835fe00fcfba207  QProtocol.ini
```
