"""Patch only launcher slot 6 and build the one-bit Bible integration bundle."""
from pathlib import Path
import hashlib
import json
import shutil
import struct

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build" / "integration"
BASE = "598b2d47702ef3a1ba8de5bdbd125ca33d99fdcce9deb94732758c3c64c86d3b"
BASE_ICON = "75483fdd1c05f6b16715559f40b479c9484e4909b2a4ec4f4971cf35f6ca151d"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def word(value):
    return struct.pack("<I", value)


def patch(data, offset, old, new):
    assert len(old) == len(new)
    assert data[offset:offset + len(old)] == old, (hex(offset), data[offset:offset + len(old)].hex())
    data[offset:offset + len(old)] = new


def make_icon(path):
    im = Image.new("RGBA", (40, 40), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # Open book with a centered cross. All geometry is hard 1-bit.
    d.line((3, 8, 18, 5, 18, 34, 3, 31, 3, 8), fill="black", width=2)
    d.line((37, 8, 22, 5, 22, 34, 37, 31, 37, 8), fill="black", width=2)
    d.line((20, 6, 20, 35), fill="black", width=1)
    d.line((9, 13, 16, 13), fill="black", width=2)
    d.line((12, 10, 12, 20), fill="black", width=2)
    d.line((25, 13, 33, 13), fill="black", width=1)
    d.line((25, 18, 33, 18), fill="black", width=1)
    d.line((7, 27, 16, 29), fill="black", width=1)
    d.line((24, 29, 34, 27), fill="black", width=1)
    im.save(path)


def main():
    src = ROOT.parent / "C1Mota" / "build" / "integration" / "mpenMain.mota"
    if digest(src) != BASE:
        raise SystemExit("Unexpected base launcher; do not overwrite newer integration")
    data = bytearray(src.read_bytes())
    OUT.mkdir(parents=True, exist_ok=True)
    # onClickPet -> penv2::mpensystem("/usr/data/h")
    patch(data, 0x30E58C, bytes.fromhex("a0ffbd2701000524070004245000b0af"),
          b"".join(word(w) for w in (0x3C040071, 0x2484E59C, 0x08131BE8, 0)))
    patch(data, 0x30E59C, bytes.fromhex("5c00bfaf5800b2afd3ac170c"), b"/usr/data/h\0")
    patch(data, 0x37CB7C, "单词小镇".encode(), "圣经".encode() + b"\0" * 6)
    patch(data, 0x37CBBC, b"TWSDMA", b"TWSDMH")
    (OUT / "mpenMain.bible").write_bytes(data)
    make_icon(OUT / "ic_desktop_cwxz.png")
    for name, source in (
        ("c1bible", ROOT / "build" / "c1bible"),
        ("bible.dat", ROOT / "build" / "bible.dat"),
        ("font12.bin", ROOT / "font12.bin"),
        ("width12.bin", ROOT / "width12.bin"),
    ):
        shutil.copy2(source, OUT / name)
    for name in ("launch-bible.sh", "install.sh"):
        (OUT / name).write_bytes((ROOT / "scripts" / name).read_bytes().replace(b"\r\n", b"\n"))
    manifest = {
        "BASE": BASE,
        "BASE_ICON": BASE_ICON,
        "LAUNCHER": digest(OUT / "mpenMain.bible"),
        "ICON": digest(OUT / "ic_desktop_cwxz.png"),
        "APP": digest(OUT / "c1bible"),
        "DATA": digest(OUT / "bible.dat"),
        "FONT": digest(OUT / "font12.bin"),
        "WIDTH": digest(OUT / "width12.bin"),
        "WRAPPER": digest(OUT / "launch-bible.sh"),
    }
    (OUT / "manifest.env").write_text("".join(f"{k}={v}\n" for k, v in manifest.items()), newline="\n")
    (OUT / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", newline="\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
