#!/usr/bin/env python3
"""Build the hash-locked C1-Slim Wawa Rich launcher integration."""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import struct
import zlib
from pathlib import Path


PORT_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE = PORT_ROOT.parents[1]
OUTPUT = PORT_ROOT / "build" / "rich-integration"
BASE_LAUNCHER_HASH = "133a46401d7393f9c577c7240ad9625698ffc294bda9c0314fb267b524683cce"
BASE_ICON_HASH = "eca390f8f616f13083d11bb20a86047c5af5e6fec019065fe86b0aa433f1e93b"


def digest_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def digest_file(path: Path) -> str:
    return digest_bytes(path.read_bytes())


def patch_exact(image: bytearray, offset: int, expected: bytes, replacement: bytes) -> None:
    if len(expected) != len(replacement):
        raise ValueError("patch lengths differ")
    actual = bytes(image[offset : offset + len(expected)])
    if actual != expected:
        raise RuntimeError(
            f"unexpected bytes at 0x{offset:x}: expected={expected.hex()} actual={actual.hex()}"
        )
    image[offset : offset + len(replacement)] = replacement


def mips_word(value: int) -> bytes:
    return struct.pack("<I", value)


def make_icon(path: Path) -> None:
    # Deliberately aliased: every visible pixel is solid black and the rest is
    # transparent, matching the C1-Slim panel's true one-bit output.
    width = height = 40
    pixels = [[False] * width for _ in range(height)]

    def point(x: int, y: int, thickness: int = 1) -> None:
        for yy in range(y, y + thickness):
            for xx in range(x, x + thickness):
                if 0 <= xx < width and 0 <= yy < height:
                    pixels[yy][xx] = True

    def line(x0: int, y0: int, x1: int, y1: int, thickness: int = 1) -> None:
        dx, sx = abs(x1 - x0), 1 if x0 < x1 else -1
        dy, sy = -abs(y1 - y0), 1 if y0 < y1 else -1
        error = dx + dy
        while True:
            point(x0, y0, thickness)
            if x0 == x1 and y0 == y1:
                break
            twice = 2 * error
            if twice >= dy:
                error += dy
                x0 += sx
            if twice <= dx:
                error += dx
                y0 += sy

    # Two square eyes and a broad smiling head stay legible at 40x40 without
    # antialiasing or simulated grey.
    for x in (6, 25):
        line(x, 6, x + 9, 6, 2)
        line(x, 6, x, 15, 2)
        line(x + 9, 6, x + 9, 15, 2)
    point(10, 10, 3)
    point(29, 10, 3)
    outline = [(7, 14), (4, 20), (5, 29), (11, 35), (28, 35), (35, 29), (36, 20), (33, 14)]
    for start, end in zip(outline, outline[1:]):
        line(*start, *end, 2)
    point(15, 21, 2)
    point(24, 21, 2)
    for start, end in zip([(11, 26), (15, 30), (20, 31), (25, 30)], [(15, 30), (20, 31), (25, 30), (29, 26)]):
        line(*start, *end, 2)

    raw = bytearray()
    for row in pixels:
        raw.append(0)  # PNG filter: None
        for black in row:
            raw.extend((0, 0, 0, 255) if black else (0, 0, 0, 0))

    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    path.write_bytes(png)


def copy_lf(source: Path, destination: Path) -> None:
    destination.write_bytes(source.read_bytes().replace(b"\r\n", b"\n"))


def main() -> None:
    private = WORKSPACE / "private"
    launcher_source = Path(os.environ.get("C1_RICH_LAUNCHER_BASE", private / "mpenMain.terminal"))
    icon_source = Path(os.environ.get("C1_RICH_ICON_BASE", private / "ic_desktop_ccyj.png"))
    runtime_source = PORT_ROOT / "build" / "c1lavax"
    event_sender_source = PORT_ROOT / "build" / "evsend"
    font_source = Path(os.environ.get("C1_LVM_FONT", private / "LVM.bin"))
    game_root = Path(os.environ.get("C1_RICH_GAME", private / "wawa-rich"))
    program_source = game_root / "Lava" / "蛙蛙大富翁.lav"
    map_source = game_root / "LavaData" / "RichMap.dat"
    picture_source = game_root / "LavaData" / "RichPic.dat"

    if digest_file(launcher_source) != BASE_LAUNCHER_HASH:
        raise RuntimeError("base launcher is not the reviewed Terminal integration")
    if digest_file(icon_source) != BASE_ICON_HASH:
        raise RuntimeError("second-icon backup does not match the connected device")

    launcher = bytearray(launcher_source.read_bytes())
    patch_exact(
        launcher,
        0x30E250,
        bytes.fromhex("a0 ff bd 27 38 00 a2 27 20 00 a7 27 54 00 b1 af"),
        b"".join(
            (
                mips_word(0x3C040071),  # lui a0, 0x71
                mips_word(0x2484E260),  # addiu a0, a0, 0xe260 -> 0x70e260
                mips_word(0x08131BE8),  # j penv2::mpensystem
                mips_word(0x00000000),  # delay-slot nop
            )
        ),
    )
    patch_exact(
        launcher,
        0x30E260,
        bytes.fromhex("50 00 b0 af 40 00 b1 27 28 00 b0 27"),
        b"/usr/data/w\0",
    )
    patch_exact(
        launcher,
        0x37CAF0,
        "查词译句".encode("utf-8") + b"\0",
        "蛙蛙富翁".encode("utf-8") + b"\0",
    )

    if OUTPUT.exists():
        shutil.rmtree(OUTPUT)
    (OUTPUT / "os" / "LavaData").mkdir(parents=True)
    (OUTPUT / "mpenMain.rich").write_bytes(launcher)
    shutil.copy2(runtime_source, OUTPUT / "c1lavax")
    shutil.copy2(event_sender_source, OUTPUT / "evsend")
    shutil.copy2(font_source, OUTPUT / "LVM.bin")
    shutil.copy2(program_source, OUTPUT / "os" / "Rich.lav")
    shutil.copy2(map_source, OUTPUT / "os" / "LavaData" / "RichMap.dat")
    shutil.copy2(picture_source, OUTPUT / "os" / "LavaData" / "RichPic.dat")
    copy_lf(PORT_ROOT / "scripts" / "launch-rich.sh", OUTPUT / "launch-rich.sh")
    copy_lf(PORT_ROOT / "scripts" / "install-rich.sh", OUTPUT / "install-rich.sh")
    make_icon(OUTPUT / "ic_desktop_ccyj.png")

    hashes = {
        "RUNTIME_SHA256": digest_file(OUTPUT / "c1lavax"),
        "EVENT_SENDER_SHA256": digest_file(OUTPUT / "evsend"),
        "FONT_SHA256": digest_file(OUTPUT / "LVM.bin"),
        "PROGRAM_SHA256": digest_file(OUTPUT / "os" / "Rich.lav"),
        "MAP_SHA256": digest_file(OUTPUT / "os" / "LavaData" / "RichMap.dat"),
        "PICTURE_SHA256": digest_file(OUTPUT / "os" / "LavaData" / "RichPic.dat"),
        "WRAPPER_SHA256": digest_file(OUTPUT / "launch-rich.sh"),
        "PATCHED_LAUNCHER_SHA256": digest_file(OUTPUT / "mpenMain.rich"),
        "PATCHED_ICON_SHA256": digest_file(OUTPUT / "ic_desktop_ccyj.png"),
    }
    environment = {
        "BASE_LAUNCHER_SHA256": BASE_LAUNCHER_HASH,
        "BASE_ICON_SHA256": BASE_ICON_HASH,
        **hashes,
    }
    (OUTPUT / "manifest.env").write_text(
        "".join(f"{key}={value}\n" for key, value in environment.items()),
        encoding="ascii",
        newline="\n",
    )
    manifest = {
        "device": "C1-Slim / MP-D261",
        "slot": 2,
        "shortcut": "W",
        "label": "蛙蛙富翁",
        "display": "296x152 one-bit, aspect-preserving nearest-neighbour",
        "files": {key.lower(): value for key, value in environment.items()},
    }
    (OUTPUT / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    print(json.dumps(manifest, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
