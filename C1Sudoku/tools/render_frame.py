"""Inspect the same packed 1-bit frame bytes written on the device."""
import sys
from pathlib import Path
from PIL import Image
for arg in sys.argv[1:]:
    path=Path(arg);data=path.read_bytes();assert len(data)==5624
    im=Image.new('1',(296,152),1)
    im.putdata([0 if data[(y//8)*296+x]&(0x80>>(y%8)) else 1 for y in range(152) for x in range(296)])
    im.save(path.with_suffix('.png'))
    im.resize((888,456),Image.Resampling.NEAREST).save(path.with_suffix('.3x.png'))
