#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <soa.h>

#include "framebuffer-constants.h"
#include "types.h"

constexpr u8 framebuffer_stride = 2 + 3; // LDA imm, STA abs
extern volatile u8 framebuffer[framebuffer_stride * FRAMEBUFFER_HEIGHT_TILES *
                                   FRAMEBUFFER_WIDTH_TILES +
                               1];

// Two columns of the frame buffer, expanded out to one byte per pixel.
extern u8 framebuffer_columns[2][FRAMEBUFFER_HEIGHT];

// Render the framebuffer columns to the framebuffer at the given column byte
// offset.
extern "C" __attribute__((leaf)) void
render_framebuffer_columns(u8 column_offset);

// A contiguous vertical span of the same color
struct Span {
  u8 color; // 0-3
  u8 size; // in pixels
};
#define SOA_STRUCT Span
#define SOA_MEMBERS MEMBER(color) MEMBER(size)
#include <soa-struct.inc>

// A buffer of spans to fill one column of the framebuffer.
class SpanBuffer {
  u8 size;
  soa::Array<Span, FRAMEBUFFER_HEIGHT> buffer;

public:
  void clear();
  void push_back(Span s);
};

extern struct SpanBuffer span_buffer;

// Render the span buffer to the leftmost framebuffer column.
extern "C" __attribute__((leaf)) void render_span_buffer_left();

// Render the span buffer to the rightmost framebuffer column.
extern "C" __attribute__((leaf)) void render_span_buffer_right();

#endif // not FRAMEBUFFER_H