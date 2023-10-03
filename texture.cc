#include "texture.h"

#include "framebuffer-constants.h"
#include "framebuffer.h"

const TextureColumn *TextureColumn::next() const {
  return reinterpret_cast<const TextureColumn *>(
      reinterpret_cast<const u8 *>(this + 1) + size * sizeof(spans[0]));
}

[[clang::noinline]] void TextureColumn::render(bool left) const {
  u8 pos = 0;
  for (u8 i = 0; i < size; ++i) {
    const TextureSpan &span = spans[i];
    u8 span_size = span.size;
    bool done = false;
    if (pos + span_size > FRAMEBUFFER_HEIGHT) {
      span_size = FRAMEBUFFER_HEIGHT - pos;
      done = true;
    }
    if (left)
      render_span_left(pos, span_size, span.color);
    else
      render_span_right(pos, span_size, span.color);
    if (done)
      break;
    pos += span_size;
  }
}