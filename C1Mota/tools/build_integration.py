"""Build the hash-locked C1-Slim Magic Tower launcher integration."""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORKSPACE = ROOT.parent
OUT = ROOT / "build" / "integration"

# Launcher currently on the device after Terminal, Rich, Sudoku and News.
BASE_LAUNCHER = "d6e85e7cca580d534b0a467c7b5fba63a8d84b745de768538a6ca8e7fcf76385"
BASE_ICON = "866af154926a1a2b151e718cc970a21a573c712c0566e839fa2b157e7ca969df"
PREVIOUS_LAUNCHER = "d6d3656a5be5bba9b1d01a4bbbecbbb7f3ae9ac7401941ff0560bf16d3a1e2e1"
PREVIOUS_WRAPPER = "624c05289d7fed24344b81fc950602f196b661017f9fd38b6bc0521c87f6265f"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def patch(blob: bytearray, offset: int, old: bytes, new: bytes) -> None:
    if len(old) != len(new):
        raise ValueError(f"patch length mismatch at {offset:#x}")
    actual = bytes(blob[offset : offset + len(old)])
    if actual != old:
        raise RuntimeError(
            f"unexpected launcher bytes at {offset:#x}: {actual.hex()} != {old.hex()}"
        )
    blob[offset : offset + len(new)] = new


def mips_word(value: int) -> bytes:
    return struct.pack("<I", value)


def make_icon(path: Path) -> None:
    # Crisp one-bit tower icon: no antialiasing or grey pixels.
    width = height = 40
    pixels = bytearray(width * height * 4)

    def pixel(x: int, y: int, color: tuple[int, int, int, int]) -> None:
        if 0 <= x < width and 0 <= y < height:
            offset = (y * width + x) * 4
            pixels[offset : offset + 4] = bytes(color)

    def rectangle(box: tuple[int, int, int, int], color: tuple[int, int, int, int],
                  outline: bool = False, thickness: int = 1) -> None:
        left, top, right, bottom = box
        for y in range(top, bottom + 1):
            for x in range(left, right + 1):
                if not outline or x < left + thickness or x > right - thickness or y < top + thickness or y > bottom - thickness:
                    pixel(x, y, color)

    black = (0, 0, 0, 255)
    white = (255, 255, 255, 255)
    rectangle((7, 11, 32, 37), black, outline=True, thickness=2)
    for x in (7, 17, 27):
        rectangle((x, 5, x + 5, 12), black)
    rectangle((14, 26, 25, 37), black)
    rectangle((17, 29, 22, 37), white)
    rectangle((10, 17, 29, 17), black)
    rectangle((10, 22, 29, 22), black)
    rectangle((10, 12, 10, 16), black)
    rectangle((20, 12, 20, 16), black)
    rectangle((29, 12, 29, 16), black)

    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    raw = b"".join(b"\0" + bytes(pixels[y * width * 4 : (y + 1) * width * 4]) for y in range(height))
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    path.write_bytes(png)


def main() -> None:
    source_launcher = Path(os.environ.get("C1_LAUNCHER_BASE", WORKSPACE / "private" / "mpenMain.pre-mota"))
    if digest(source_launcher) != BASE_LAUNCHER:
        raise RuntimeError("unexpected launcher base; refusing to overwrite newer integrations")

    launcher = bytearray(source_launcher.read_bytes())

    # Slot 5: replace on_click_myBooks with mpensystem('/usr/data/m').
    patch(
        launcher,
        0x30E924,
        bytes.fromhex("90ffbd276c00bfaf6800b2af6400b1af"),
        b"".join(
            mips_word(word)
            for word in (0x3C040071, 0x2484E934, 0x08131BE8, 0x00000000)
        ),
    )
    patch(
        launcher,
        0x30E934,
        bytes.fromhex("84d0140c6000b0af71d2140c"),
        b"/usr/data/m\0",
    )
    patch(launcher, 0x37CBBC, b"TWSDZA", b"TWSDMA")

    # The stock fifth-entry title reuses the "自建词书" tail inside a longer
    # DIY-book message instead of having its own desktop string.  Keep that
    # message intact, place "魔塔" in the seven-byte alignment gap after the
    # sixth title, and redirect only slot 5's title construction to the gap.
    patch(launcher, 0x37CB89, b"\0" * 7, "魔塔".encode("utf-8") + b"\0")
    patch(launcher, 0x30F024, bytes.fromhex("74000c3c"), bytes.fromhex("78000c3c"))
    patch(launcher, 0x30F048, bytes.fromhex("f4598b25"), bytes.fromhex("89cb8b25"))
    patch(launcher, 0x30F0CC, bytes.fromhex("f459928d"), bytes.fromhex("89cb928d"))

    # Disable cppMain's automatic update check only.  The settings page has a
    # separate listener and remains available for deliberate manual updates.
    patch(
        launcher,
        0x2D09BC,
        bytes.fromhex("68ffbd2710000424"),
        mips_word(0x03E00008) + mips_word(0x00000000),  # jr ra; nop
    )

    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "mpenMain.mota").write_bytes(launcher)
    make_icon(OUT / "ic_desktop_zjcs.png")

    copies = {
        WORKSPACE / "C1LavaX" / "port" / "build" / "c1lavax": "c1lavax",
        WORKSPACE / "C1LavaX" / "port" / "assets" / "LVM.bin": "LVM.bin",
        WORKSPACE / "LavaXOS" / "魔塔.lav": "Mota.lav",
        WORKSPACE / "LavaXOS" / "LavaData" / "MOTA.dat": "MOTA.dat",
        ROOT / "scripts" / "launch-mota.sh": "launch-mota.sh",
        ROOT / "scripts" / "install.sh": "install.sh",
    }
    for source, name in copies.items():
        if not source.is_file():
            raise FileNotFoundError(source)
        destination = OUT / name
        if source.suffix == ".sh":
            destination.write_bytes(source.read_bytes().replace(b"\r\n", b"\n"))
        else:
            shutil.copy2(source, destination)

    manifest = {
        "BASE": BASE_LAUNCHER,
        "BASE_ICON": BASE_ICON,
        "PREVIOUS": PREVIOUS_LAUNCHER,
        "PREVIOUS_WRAPPER": PREVIOUS_WRAPPER,
        "LAUNCHER": digest(OUT / "mpenMain.mota"),
        "ICON": digest(OUT / "ic_desktop_zjcs.png"),
        "RUNTIME": digest(OUT / "c1lavax"),
        "FONT": digest(OUT / "LVM.bin"),
        "PROGRAM": digest(OUT / "Mota.lav"),
        "DATA": digest(OUT / "MOTA.dat"),
        "WRAPPER": digest(OUT / "launch-mota.sh"),
        "INSTALLER": digest(OUT / "install.sh"),
    }
    (OUT / "manifest.env").write_text(
        "".join(f"{key}={value}\n" for key, value in manifest.items()),
        encoding="ascii",
        newline="\n",
    )
    (OUT / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
