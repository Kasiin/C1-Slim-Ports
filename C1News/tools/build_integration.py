"""Hash-locked slot 4 news integration on top of Terminal/Rich/Sudoku."""
from pathlib import Path
import hashlib,json,struct,shutil
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/integration'
BASE='78f4a953b29a5749c1504c6c564f156795b0549b0c9502be048d5b4788c7962c'
BASE_ICON='d81e896b8cc14a2edc1a6c1098225c796dde3d48cf10f0f6f0be25d74ce6725b'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def patch(b,offset,old,new):
    assert len(old)==len(new)
    assert b[offset:offset+len(old)]==old,hex(offset)
    b[offset:offset+len(old)]=new
def main():
    src=ROOT.parent/'C1Sudoku/build/integration/mpenMain.sudoku'
    assert digest(src)==BASE,'Unexpected launcher; refusing to overwrite other changes'
    b=bytearray(src.read_bytes());OUT.mkdir(parents=True,exist_ok=True)
    # Fourth-entry on_click_suguo -> mpensystem('/usr/data/d').
    patch(b,0x30eb78,bytes.fromhex('90ffbd276c00bfaf6800b2af6400b1af'),
          b''.join(struct.pack('<I',w) for w in (0x3c040071,0x2484eb88,0x08131be8,0)))
    patch(b,0x30eb88,bytes.fromhex('84d0140c6000b0af71d2140c'),b'/usr/data/d\0')
    patch(b,0x37cb4c,'单词速过'.encode(),'简报'.encode()+b'\0'*6)
    patch(b,0x37cbbc,b'TWSAZD',b'TWSDZA')
    (OUT/'mpenMain.news').write_bytes(b)
    # Bilevel newspaper icon, matching the existing stock 40px icon system.
    im=Image.new('RGBA',(40,40),(0,0,0,0));d=ImageDraw.Draw(im)
    d.rectangle((4,3,35,36),outline='black',width=2)
    d.line((0,10,0,34,4,38,35,38,38,35,38,10),fill='black',width=2)
    d.rectangle((8,7,17,16),fill='black')
    for y in (8,12,16):d.line((22,y,31,y),fill='black',width=1)
    for y in (22,27,32):d.line((8,y,31,y),fill='black',width=1)
    im.save(OUT/'ic_desktop_dcsg.png')
    for name in ('c1news','cacert.pem'):shutil.copy2(ROOT/'build'/name,OUT/name)
    for name in ('launch-news.sh','install.sh'):(OUT/name).write_bytes((ROOT/'scripts'/name).read_bytes().replace(b'\r\n',b'\n'))
    manifest={'BASE':BASE,'PREVIOUS':'a0009f7fe0b85a86171351a31a96eb7ffb6b59e7450c6092e265d929920408e7','BASE_ICON':BASE_ICON,'LAUNCHER':digest(OUT/'mpenMain.news'),'ICON':digest(OUT/'ic_desktop_dcsg.png'),
              'APP':digest(OUT/'c1news'),'CA':digest(OUT/'cacert.pem'),'WRAPPER':digest(OUT/'launch-news.sh')}
    (OUT/'manifest.env').write_text(''.join(f'{k}={v}\n' for k,v in manifest.items()),newline='\n')
    (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2))
if __name__=='__main__':main()
