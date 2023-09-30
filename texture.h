#ifndef TEXTURE_H
#define TEXTURE_H

#include "types.h"

class TextureSpan {
  u8 color;
  u8 size;
};

class TextureColumn {
  u8 size;
  // An array of length `size`.
  TextureSpan *spans;

public:
  const TextureSpan& operator[](u8 index) const;
};

class Texture {
  u8 width;
  u8 height;

  // An array of length `width`.
  TextureColumn *columns;

public:
  const TextureColumn& operator[](u8 index) const;
};

#endif // not TEXTURE_H