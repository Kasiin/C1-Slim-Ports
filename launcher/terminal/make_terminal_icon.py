#!/usr/bin/env python3
"""Create a deterministic 40x40 RGBA terminal icon matching the stock set."""

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "artifacts" / "stock-terminal-integration" / "ic_desktop_dcjy.png"
SCALE = 4


def scaled(points):
    return tuple(value * SCALE for value in points)


canvas = Image.new("RGBA", (40 * SCALE, 40 * SCALE), (0, 0, 0, 0))
draw = ImageDraw.Draw(canvas)
ink = (0, 0, 0, 255)

# Match the friend's stock-style icon: a plain square terminal window with a
# dark >_ prompt. The device renders PNG colors directly on a light background.
draw.rectangle(scaled((5, 7, 35, 33)), outline=ink, width=2 * SCALE)
draw.line(scaled((12, 16, 18, 21, 12, 26)), fill=ink, width=2 * SCALE, joint="curve")
draw.line(scaled((22, 26, 29, 26)), fill=ink, width=2 * SCALE)

OUTPUT.parent.mkdir(parents=True, exist_ok=True)
canvas.resize((40, 40), Image.Resampling.LANCZOS).save(OUTPUT, format="PNG", optimize=True)
print(OUTPUT)
