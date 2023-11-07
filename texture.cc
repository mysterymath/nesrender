#include "texture.h"

#include <stdio.h>

#include "framebuffer-constants.h"
#include "framebuffer.h"
#include "log.h"
#include "logo.h"

const TextureColumn *TextureColumn::next() const {
  return reinterpret_cast<const TextureColumn *>(
      reinterpret_cast<const u8 *>(this + 1) + size * sizeof(spans[0]));
}

// In 1/256 pixel
extern u16 y_start;
extern u16 y_end;
extern Log v_scale;

[[clang::noinline]] void TextureColumn::render(bool left) const {
  u8 span_idx = 0;
  for (u16 y_pos = y_start; y_pos < y_end;) {
    const TextureSpan &span = spans[span_idx];
    u16 y_size = span.size * v_scale; // in 1/256 pixel
    if (y_size >= y_end - y_pos)
      y_size = y_end - y_pos;

    if (left)
      render_span_left(y_pos >> 8, y_size >> 8, span.color);
    else
      render_span_right(y_pos >> 8, y_size >> 8, span.color);

    y_pos += y_size;
    if (++span_idx == size)
      span_idx = 0;
  }
}
