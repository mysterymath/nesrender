#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "framebuffer-constants.h"
#include "types.h"

constexpr u8 framebuffer_stride = 2 + 3; // LDA imm, STA abs
extern volatile u8 framebuffer[framebuffer_stride * FRAMEBUFFER_HEIGHT_TILES *
                                   FRAMEBUFFER_WIDTH_TILES +
                               1];

extern u8 framebuffer_columns[2][FRAMEBUFFER_HEIGHT];

extern "C" __attribute__((leaf)) void render_framebuffer_columns(u8 column_offset);

#endif // not FRAMEBUFFER_H