from PIL import Image, ImageOps
import sys

pal = [0, 0, 0] + [0x73, 0x0b, 0x00] + [0xc7, 0x2e, 0x00] + [0xff, 0x77, 0x57] + [0,0,0] * 252
palIm = Image.new('P', (1,1))
palIm.putpalette(pal)

#target_size = (64, 42)
#target_size = (128, 84)
target_size = (256, 168)

with Image.open(sys.argv[1]) as im:
    new_width = int(target_size[1] / im.height * im.width)
    new_height = int(target_size[0] / im.width * im.height)
    if new_width > target_size[0]:
      new_size = (target_size[0], new_height)
    else:
      new_size = (new_width, target_size[1])
    im = im.resize(new_size, Image.Resampling.LANCZOS)
    im = ImageOps.grayscale(im)
    im = ImageOps.colorize(im, (0,0,0), (255, 0, 0))
    im = im.quantize(palette=palIm, dither=Image.Dither.NONE)
    im.save(sys.argv[2])
