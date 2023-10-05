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

extern u16 y_start;
extern u16 y_end;
extern u16 v_offset;
extern Log v_scale;
extern u16 v_scale_lin;

[[clang::noinline]] void TextureColumn::render(bool left) const {
  u16 y_pos = y_start;
  u16 v_pos = v_offset;

  y_pos += 128;
  if (y_pos >= y_end)
    return;
  v_pos += v_scale_lin / 2;
  if (v_pos >= logo.height << 8)
    v_pos -= logo.height << 8;

  u8 span_idx = 0;
  u16 span_v_begin = 0;
  while (y_pos < y_end) {
    if (span_v_begin > v_pos) {
      span_idx = 0;
      span_v_begin = 0;
    }

    for (; span_idx < size; ++span_idx) {
      const TextureSpan &span = spans[span_idx];
      u16 span_v_end = span_v_begin + Log(span.size) * v_scale;
      if (v_pos < span_v_end)
        break;
      span_v_begin = span_v_end;
    }

    const TextureSpan &span = spans[span_idx];
    u16 start_y_pos = y_pos;
    u16 span_v_end = span_v_begin + Log(span.size) * v_scale;
    u8 draw_length = 0;
    while (y_pos < y_end && v_pos < span_v_end) {
      draw_length++;
      y_pos += 256;
      v_pos += v_scale_lin;
    }

    if (left)
      render_span_left(start_y_pos >> 8, draw_length, span.color);
    else
      render_span_right(start_y_pos >> 8, draw_length, span.color);
  }
}
