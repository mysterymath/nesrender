#ifndef TEXTURE_H
#define TEXTURE_H

#include "types.h"

struct TextureSpan {
  u8 color;
  u8 size;
};

struct TextureColumn {
  u8 size;
  TextureSpan spans[];
};

struct Texture {
  u8 width;
  u8 height;

  TextureColumn columns[];
};

#endif // not TEXTURE_H
