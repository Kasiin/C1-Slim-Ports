"""Build a full BMP, strictly bilevel Unifont for unpredictable feed text."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
font=ImageFont.truetype(str(ROOT.parent/'C1Sudoku/assets/unifont-16.0.01.otf'),16)
out=bytearray(65536*32)
for cp in range(32,65536):
    if 0xd800<=cp<=0xdfff:continue
    im=Image.new('1',(16,16));ImageDraw.Draw(im).text((0,0),chr(cp),font=font,fill=1,anchor='la')
    for y in range(16):
        v=sum(0x8000>>x for x in range(16) if im.getpixel((x,y)))
        out[cp*32+y*2:cp*32+y*2+2]=v.to_bytes(2,'big')
(ROOT/'font.bin').write_bytes(out)
print('BMP 1-bit font:',len(out),'bytes')
small=ImageFont.truetype(str(ROOT/'assets/wqy14.ttf'),14)
out=bytearray(65536*32)
widths=bytearray(65536)
for cp in range(32,65536):
    if 0xd800<=cp<=0xdfff:continue
    # Native bitmap outlines, rendered without antialiasing or resampling.
    widths[cp]=min(16,max(1,round(small.getlength(chr(cp)))))
    if cp>=0x2e80:widths[cp]=max(14,widths[cp])
    im=Image.new('1',(16,16))
    ImageDraw.Draw(im).text((0,0),chr(cp),font=small,fill=1,anchor='la')
    for y in range(16):
        v=sum(0x8000>>x for x in range(16) if im.getpixel((x,y)))
        out[cp*32+y*2:cp*32+y*2+2]=v.to_bytes(2,'big')
(ROOT/'font14.bin').write_bytes(out)
(ROOT/'width14.bin').write_bytes(widths)
print('Native small font:',len(out),'bytes')
# The 15px bitmap outlines have an 1800-unit em and a 100-unit pixel grid.
# Rendering at 18ppem maps each original pixel exactly to one screen pixel.
# Its visible CJK glyph is 15px; offset -1 preserves the Latin descenders.
reader=ImageFont.truetype(str(ROOT/'assets/wqy15.ttf'),18)
out=bytearray(65536*32);widths=bytearray(65536)
for cp in range(32,65536):
    if 0xd800<=cp<=0xdfff:continue
    widths[cp]=min(16,max(1,round(reader.getlength(chr(cp)))))
    if cp>=0x2e80:widths[cp]=max(16,widths[cp])
    im=Image.new('1',(16,16))
    ImageDraw.Draw(im).text((0,-1),chr(cp),font=reader,fill=1,anchor='la')
    for y in range(16):
        v=sum(0x8000>>x for x in range(16) if im.getpixel((x,y)))
        out[cp*32+y*2:cp*32+y*2+2]=v.to_bytes(2,'big')
(ROOT/'font15.bin').write_bytes(out)
(ROOT/'width15.bin').write_bytes(widths)
print('Pixel-aligned 15px reader:',len(out),'bytes')
