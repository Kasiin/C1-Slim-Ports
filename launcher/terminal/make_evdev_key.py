#!/usr/bin/env python3
"""Create a 32-bit Linux evdev key press/release stream for device testing."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


def event(event_type: int, code: int, value: int) -> bytes:
    # MP-D261 userspace is 32-bit: timeval (2 x uint32), type/code, value.
    return struct.pack("<IIHHi", 0, 0, event_type, code, value)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("key_code", type=int)
    parser.add_argument(
        "--action", choices=("press", "release", "tap"), default="tap"
    )
    args = parser.parse_args()

    states = {
        "press": (1,),
        "release": (0,),
        "tap": (1, 0),
    }[args.action]
    stream = b"".join(
        event(1, args.key_code, value) + event(0, 0, 0) for value in states
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(stream)
    print(
        f"wrote {len(stream)} bytes for key code {args.key_code} ({args.action})"
    )


if __name__ == "__main__":
    main()
