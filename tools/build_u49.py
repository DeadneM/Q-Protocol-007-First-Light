#!/usr/bin/env python3
"""Build Q Protocol v0.8.12U49 from the canonical U33 QProtocol.asi.

No external dependencies are required.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import sys

BASE_SHA256 = "a0ac354d7c7f2c96ceae2fa3da6cadc0756d325d9fb796a4da2083a223f10dce"
OUTPUT_SHA256 = "ba119fd8b781b15c1bc41bf393cddcd5dc7a3836ab9081426e4e6a01245a3a03"

PATCHES = [
    (
        0x00007BD4,
        bytes.fromhex("48 8B 05 FD BB 00 00"),
        bytes.fromhex("E9 37 DB 20 00 90 90"),
        "AUTO weapon success path -> U49 trampoline (RVA 0x87D4)",
    ),
    (
        0x000101F0,
        bytes.fromhex("33 33"),
        bytes.fromhex("34 39"),
        "embedded build banner U33 -> U49",
    ),
    (
        0x00012F10,
        bytes(17),
        bytes.fromhex(
            "E8 EB FC FF FF "
            "48 8B 05 BC E0 DF FF "
            "E9 BA 24 DF FF"
        ),
        "trampoline in existing .q25 executable cave (RVA 0x216310)",
    ),
]


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> "NoReturn":
    print(f"[ERROR] {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build Q Protocol v0.8.12U49 from canonical U33 QProtocol.asi"
    )
    parser.add_argument("input", type=Path, help="canonical U33 QProtocol.asi")
    parser.add_argument(
        "-o", "--output", type=Path, default=Path("QProtocol.asi"),
        help="output path (default: QProtocol.asi)",
    )
    args = parser.parse_args()

    raw = args.input.read_bytes()
    actual_base = sha256(raw)
    if actual_base != BASE_SHA256:
        fail(
            "wrong base ASI. Expected canonical U33 SHA-256 "
            f"{BASE_SHA256}, got {actual_base}"
        )

    image = bytearray(raw)
    for offset, expected, replacement, note in PATCHES:
        current = bytes(image[offset:offset + len(expected)])
        if current != expected:
            fail(
                f"preimage mismatch at file offset 0x{offset:X} ({note}). "
                f"Expected {expected.hex(' ')}, got {current.hex(' ')}"
            )
        image[offset:offset + len(replacement)] = replacement
        print(f"[OK] 0x{offset:08X}: {note}")

    result = bytes(image)
    actual_output = sha256(result)
    if actual_output != OUTPUT_SHA256:
        fail(
            "patched output hash mismatch. Expected "
            f"{OUTPUT_SHA256}, got {actual_output}"
        )

    args.output.write_bytes(result)
    print(f"[OK] Wrote {args.output}")
    print(f"[OK] SHA-256 {actual_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
