"""Subset the device's Unifont into binary-only 16px glyphs."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
font=ImageFont.truetype(str(ROOT/'assets/unifont-16.0.01.otf'),16)
chars=set(chr(i) for i in range(32,127))
chars.update(c for c in (ROOT/'src/main.cpp').read_text(encoding='utf8') if ord(c)>127)
out=['// Generated from device Unifont (GPL font exception).\n#include <cstdint>\nstruct Glyph { uint32_t code; int width; uint16_t rows[16]; };\nstatic const Glyph FONT[] = {\n']
for c in sorted(chars):
    w=8 if ord(c)<128 else 16
    im=Image.new('1',(16,16));ImageDraw.Draw(im).text((0,0),c,font=font,fill=1,anchor='lt')
    # Unifont has a full em-height bitmap; baseline alignment must be shared
    # across digits and Chinese rather than trimming each individual glyph.
    im=Image.new('1',(16,16));ImageDraw.Draw(im).text((0,0),c,font=font,fill=1,anchor='la')
    rows=[sum((0x8000>>x) for x in range(w) if im.getpixel((x,y))) for y in range(16)]
    out.append('{%d,%d,{%s}},\n'%(ord(c),w,','.join(hex(x) for x in rows)))
out.append('};\n');(ROOT/'assets/font.h').write_text(''.join(out))
print('font glyphs:',len(chars))
