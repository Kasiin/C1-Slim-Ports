"""Patch only reviewed third-entry callback/icon/label and accelerator table."""
from pathlib import Path
import hashlib,json,struct,shutil
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/integration'
BASE='80686d2c7673098fab183045de96a9ebf13b6194eda41ece0d87998ce5d97ae9'
ICON='e62129da8cefc26b6c11a161177060a928f5dc733651bfb278d8e32ad2440298'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def word(w):return struct.pack('<I',w)
def patch(b,offset,old,new):
    assert len(old)==len(new)
    assert b[offset:offset+len(old)]==old,(hex(offset),b[offset:offset+len(old)].hex())
    b[offset:offset+len(old)]=new
def main():
    src=ROOT.parent/'C1LavaX/port/build/rich-integration/mpenMain.rich'
    assert digest(src)==BASE,'Unexpected base launcher; do not overwrite newer integration'
    b=bytearray(src.read_bytes());OUT.mkdir(parents=True,exist_ok=True)
    patch(b,0x30dffc,bytes.fromhex('90ffbd276c00bfaf6800b2af6400b1af'),
          b''.join(word(w) for w in (0x3c040071,0x2484e00c,0x08131be8,0)))
    patch(b,0x30e00c,bytes.fromhex('84d0140c6000b0af71d2140c'),b'/usr/data/s\0')
    patch(b,0x37cb2c,'单词听写'.encode(), '数独'.encode()+b'\0'*6)
    patch(b,0x37cbbc,b'TWEASD',b'TWSAZD')
    (OUT/'mpenMain.sudoku').write_bytes(b)
    im=Image.new('RGBA',(40,40),(0,0,0,0));d=ImageDraw.Draw(im)
    for k in range(4):
        p=2+k*12
        d.line((p,2,p,38),fill='black',width=2 if k in (0,3) else 1)
        d.line((2,p,38,p),fill='black',width=2 if k in (0,3) else 1)
    font=ImageFont.truetype(str(ROOT/'assets/unifont-16.0.01.otf'),16)
    # Pixel font rendered directly in bilevel mode (no anti-aliasing).
    mask=Image.new('1',(40,40));md=ImageDraw.Draw(mask)
    for x,y,c in [(5,1,'1'),(29,13,'3'),(17,25,'9')]:md.text((x,y),c,font=font,fill=1)
    for y in range(40):
        for x in range(40):
            if mask.getpixel((x,y)):im.putpixel((x,y),(0,0,0,255))
    im.save(OUT/'ic_desktop_dctx.png')
    shutil.copy2(ROOT/'build/c1sudoku',OUT/'c1sudoku')
    for name in ('launch-sudoku.sh','install.sh'):
        (OUT/name).write_bytes((ROOT/'scripts'/name).read_bytes().replace(b'\r\n',b'\n'))
    manifest={'BASE':BASE,'BASE_ICON':ICON,'LAUNCHER':digest(OUT/'mpenMain.sudoku'),
              'ICON':digest(OUT/'ic_desktop_dctx.png'),'APP':digest(OUT/'c1sudoku'),
              'WRAPPER':digest(OUT/'launch-sudoku.sh')}
    (OUT/'manifest.env').write_text(''.join(f'{k}={v}\n' for k,v in manifest.items()),newline='\n')
    (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2))
if __name__=='__main__':main()
