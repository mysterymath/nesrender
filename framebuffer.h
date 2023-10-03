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

extern "C" __attribute__((leaf)) u8 render_span_left(u8 pos, u8 length,
                                                     u8 color);

extern "C" __attribute__((leaf)) u8 render_span_right(u8 pos, u8 length,
                                                      u8 color);

#endif // not FRAMEBUFFER_H