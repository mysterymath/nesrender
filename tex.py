from PIL import Image
from pathlib import Path
import sys

im = Image.open(sys.argv[1])
im = im.transpose(Image.Transpose.TRANSPOSE)
stem = Path(sys.argv[1]).stem

print('#include "stdint.h"')
print('#include "tex.h"')
print(f'static const uint8_t {stem}_colors[] = {{ {",".join(str(x) for x in im.getdata())} }};')
# Note: The image has already been transposed.
print(f'extern const Texture {stem} = {{{im.height}, {im.width}, {stem}_colors}};')
