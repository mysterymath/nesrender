from PIL import Image
import sys

pal = [0, 0, 0] + [0x73, 0x0b, 0x00] + [0xc7, 0x2e, 0x00] + [0x00, 0x3c, 0x76] + [0,0,0] * 252
palIm = Image.new('P', (1,1))
palIm.putpalette(pal)

with Image.open(sys.argv[1]) as im:
    im = im.resize((64,42), Image.Resampling.LANCZOS)
    im = im.quantize(palette=palIm, dither=Image.Dither.NONE)
    im.save(sys.argv[2])
