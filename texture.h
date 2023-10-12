#ifndef TEXTURE_H
#define TEXTURE_H

#include "log.h"
#include "types.h"

struct TextureSpan {
  u8 color;
  Log size; // in 1/256 texel
};

struct TextureColumn {
  u8 size;
  TextureSpan spans[];

  // Return the texture column to the right of this one.
  const TextureColumn *next() const;

  // Render the column at a 1:1 scale to the span buffer.
  void render(bool left) const;
};

struct Texture {
  u8 width;
  u8 height;

  TextureColumn columns[];
};

#endif // not TEXTURE_H
