from PIL import Image, ImageOps
import sys

pal = [0, 0, 0] + [0x73, 0x0b, 0x00] + [0xc7, 0x2e, 0x00] + [0xff, 0x77, 0x57] + [0,0,0] * 252
palIm = Image.new('P', (1,1))
palIm.putpalette(pal)

#target_size = (64, 42)
target_size = (128, 84)

image_filename = sys.argv[1]
name = sys.argv[2]

with Image.open(image_filename) as im:
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

    pixels = list(im.getdata())
    columns = []
    for x in range(im.width):
      spans = [[pixels[x], 1]]
      for y in range(1, im.height):
        color = pixels[y*im.width + x]
        if color == spans[-1][0]:
            spans[-1][1] += 1
        else:
            spans.append([color, 1])
      columns.append(spans)

    with open(name + '.h', 'w') as f:
      macro = f"{name.upper()}_H"
      print(f"#ifndef {macro}", file=f)
      print(f"#define {macro}", file=f)
      print("#include \"texture.h\"", file=f)
      print(f"extern const u8 {name}_bytes[];", file=f)
      print(f"#define {name} (*reinterpret_cast<const Texture*>({name}_bytes))", file=f)
      print(f"#endif // not {macro}", file=f)
    with open(name + '.cc', 'w') as f:
      print(f"#include \"{name}.h\"\n", file=f)
      print("#include \"types.h\"\n", file=f)
      print(f"const u8 {name}_bytes[] = {'{'}", file=f)
      print(f"  {im.width}, {im.height},", file=f)
      for spans in columns:
        print(f"  {len(spans)},", file=f)
        for span in spans:
          print(f"    {span[0]}, {span[1]},", file=f)
      print("};", file=f)

    im.save(name + '.png')
