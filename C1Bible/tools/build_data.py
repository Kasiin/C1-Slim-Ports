"""Build a compact, memory-mappable CUVS text and pinyin search database."""
from pathlib import Path
import hashlib
import os
import re
import sqlite3
import struct
import urllib.request

from pypinyin import Style, lazy_pinyin

ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path(os.environ.get("C1BIBLE_SOURCE", ROOT / "third_party" / "cuvs.sqlite"))
SOURCE_URL = "https://raw.githubusercontent.com/midvash/bible-data/main/versions/zh/cuvs/cuvs.sqlite"
SOURCE_SHA256 = "d08201e63895cebd335f8d9673b68d3c7642e108ebd4988ff59ed8453d88d30d"
OUT = ROOT / "build" / "bible.dat"

CHAPTERS = [
    50,40,27,36,34,24,21,4,31,24,22,25,29,36,10,13,10,42,150,31,12,8,
    66,52,5,48,12,14,3,9,1,4,7,3,3,3,2,14,4,
    28,16,24,21,28,16,16,13,6,6,4,4,5,3,6,4,3,1,13,5,5,3,5,1,1,1,22,
]


def normalized_pinyin(text: str):
    def keep_ascii(chars: str):
        return "".join(c.lower() for c in chars if c.isascii() and c.isalnum())
    raw = lazy_pinyin(text, style=Style.NORMAL, strict=False, errors=keep_ascii)
    parts = [re.sub(r"[^a-z0-9]", "", value.lower()) for value in raw]
    parts = [value for value in parts if value]
    return ("'" + "'".join(parts) + "'").encode("ascii"), set(parts)


def main() -> None:
    if not SOURCE.exists():
        SOURCE.parent.mkdir(parents=True, exist_ok=True)
        temporary = SOURCE.with_suffix(".download")
        print(f"Downloading reviewed CUVS source to {SOURCE}")
        urllib.request.urlretrieve(SOURCE_URL, temporary)
        temporary.replace(SOURCE)
    digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    if digest != SOURCE_SHA256:
        raise SystemExit(f"Unexpected CUVS source hash: {digest}")

    db = sqlite3.connect(SOURCE)
    rows = list(db.execute(
        "SELECT book_id,chapter,number,text FROM verses ORDER BY book_id,chapter,number"
    ))
    db.close()
    if len(rows) != 31021:
        raise SystemExit(f"Expected 31021 verses, got {len(rows)}")
    actual = [0] * 66
    for book, chapter, _, _ in rows:
        if not 1 <= book <= 66:
            raise SystemExit(f"Invalid book id: {book}")
        actual[book - 1] = max(actual[book - 1], chapter)
    if actual != CHAPTERS:
        raise SystemExit("Chapter counts do not match the 66-book Protestant canon")

    records = []
    texts = bytearray()
    pinyins = bytearray()
    syllables = set()
    for book, chapter, verse, text in rows:
        raw = text.strip().encode("utf-8")
        py, verse_syllables = normalized_pinyin(text)
        syllables.update(verse_syllables)
        records.append((book, chapter, verse, 0, len(texts), len(raw), len(pinyins), len(py)))
        texts.extend(raw)
        pinyins.extend(py)

    header_size = 36
    record_size = 24
    records_offset = header_size
    text_offset = records_offset + len(records) * record_size
    pinyin_offset = text_offset + len(texts)
    syllable_offset = pinyin_offset + len(pinyins)
    syllable_data = ("\n".join(sorted(syllables, key=lambda s: (-len(s), s))) + "\n").encode("ascii")
    file_size = syllable_offset + len(syllable_data)
    header = struct.pack(
        "<8s7I", b"C1BIBL2\0", 2, len(records), records_offset,
        text_offset, pinyin_offset, syllable_offset, file_size,
    )
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("wb") as f:
        f.write(header)
        for record in records:
            f.write(struct.pack("<4H4I", *record))
        f.write(texts)
        f.write(pinyins)
        f.write(syllable_data)
    if OUT.stat().st_size != file_size:
        raise SystemExit("Output length mismatch")
    print(f"Built {OUT}: {len(rows)} verses, {file_size:,} bytes")


if __name__ == "__main__":
    main()
