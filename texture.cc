#include "texture.h"

#include "framebuffer-constants.h"
#include "framebuffer.h"
#include "log.h"
#include "logo.h"

const TextureColumn *TextureColumn::next() const {
  return reinterpret_cast<const TextureColumn *>(
      reinterpret_cast<const u8 *>(this + 1) + size * sizeof(spans[0]));
}

extern u16 y_start;
extern u16 y_end;
extern u16 v_offset;
extern Log v_scale;

[[clang::noinline]] void TextureColumn::render(bool left) const {
  u16 y_pos = y_start;
  u16 v_pos = v_offset;

  y_pos += 128;
  if (y_pos >= y_end)
    return;
  v_pos += v_scale / Log::pow2(1);
  if (v_pos >= logo.height << 8)
    v_pos -= logo.height << 8;

  while (y_pos < y_end) {
    y_pos += 256;
    v_pos += v_scale;
    u16 span_v_begin = 0;
    for (u8 i = 0; i < size; ++i) {
      const TextureSpan &span = spans[i];
      u16 span_v_end = span_v_begin + span.size * v_scale;
      if (v_pos < span_v_end) {
        if (left)
          render_span_left(y_pos >> 8, 1, span.color);
        else
          render_span_right(y_pos >> 8, 1, span.color);
        break;
      }
    }
  }
}
