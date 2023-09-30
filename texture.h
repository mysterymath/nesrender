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

  // Render the column at a 1:1 scale to the span buffer.
  void render() const;
};

struct Texture {
  u8 width;
  u8 height;

  TextureColumn columns[];
};

#endif // not TEXTURE_H
