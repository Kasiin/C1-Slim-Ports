"""Build the 12px Fusion Pixel Font table used by the device reader."""
from __future__ import annotations

import hashlib
import os
import sqlite3
from pathlib import Path

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path(os.environ.get("C1BIBLE_SOURCE", ROOT / "third_party" / "cuvs.sqlite"))
FUSION = ROOT / "assets" / "fusion-pixel-12px" / "fusion-pixel-12px-proportional-zh_hans.ttf"
FALLBACK = ROOT.parent / "C1News" / "assets" / "wqy14.ttf"
FUSION_SHA256 = "1b423de0be589d159ef71af7d00530a7176e16af768a26adbc2e524e8817aad9"
FALLBACK_SHA256 = "1da8cb17abb4fd34d4cfc83be7e16baeb49f3822f9236499d29cd9ca7a4f68f6"
SIZE = 12


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def unicode_cmap(path: Path) -> set[int]:
    font = TTFont(path)
    result: set[int] = set()
    for table in font["cmap"].tables:
        if table.isUnicode():
            result.update(table.cmap)
    return result


def put_glyph(glyphs: bytearray, cp: int, width: int, image: Image.Image) -> None:
    offset = cp * 32
    for row in range(16):
        bits = sum(0x8000 >> x for x in range(width) if image.getpixel((x, row)))
        glyphs[offset + row * 2 : offset + row * 2 + 2] = bits.to_bytes(2, "big")


def main() -> None:
    if digest(FUSION) != FUSION_SHA256:
        raise SystemExit("Unexpected Fusion Pixel Font asset")
    if digest(FALLBACK) != FALLBACK_SHA256:
        raise SystemExit("Unexpected WenQuanYi fallback asset")

    db = sqlite3.connect(SOURCE)
    required = {
        ord(char)
        for (text,) in db.execute("SELECT text FROM verses")
        for char in text
        if not char.isspace() and ord(char) < 65536
    }
    db.close()

    fusion = ImageFont.truetype(str(FUSION), SIZE)
    fusion_cmap = unicode_cmap(FUSION)
    fallback = ImageFont.truetype(str(FALLBACK), SIZE)
    fallback_cmap = unicode_cmap(FALLBACK)
    glyphs = bytearray(65536 * 32)
    widths = bytearray([SIZE // 2] * 65536)

    for cp in sorted(cp for cp in fusion_cmap if 32 <= cp < 65536 and not 0xD800 <= cp <= 0xDFFF):
        char = chr(cp)
        width = min(16, max(1, round(fusion.getlength(char))))
        if cp >= 0x2E80:
            width = max(SIZE, width)
        widths[cp] = width
        image = Image.new("1", (16, 16))
        ImageDraw.Draw(image).text((0, -3), char, font=fusion, fill=1, anchor="la")
        put_glyph(glyphs, cp, width, image)

    # The reviewed CUVS source contains U+2D8D in two places where the text is
    # clearly “树墩子 / 木墩子”; retain source offsets but draw the intended glyph.
    substitutions = {0x2D8D: 0x58A9}
    unresolved = []
    fallback_count = 0
    for cp in sorted(required - fusion_cmap):
        source_cp = substitutions.get(cp, cp)
        if source_cp not in fallback_cmap:
            unresolved.append(cp)
            continue
        widths[cp] = SIZE
        image = Image.new("1", (16, 16))
        ImageDraw.Draw(image).text((0, 0), chr(source_cp), font=fallback, fill=1, anchor="la")
        put_glyph(glyphs, cp, SIZE, image)
        fallback_count += 1
    if unresolved:
        raise SystemExit("Unresolved Bible glyphs: " + ", ".join(f"U+{cp:04X}" for cp in unresolved))

    (ROOT / "font12.bin").write_bytes(glyphs)
    (ROOT / "width12.bin").write_bytes(widths)
    print(f"Built Fusion Pixel 12px font with {fallback_count} Bible fallback glyphs")


if __name__ == "__main__":
    main()
