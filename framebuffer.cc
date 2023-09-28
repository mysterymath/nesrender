#include "framebuffer.h"

// Zero-page pointers to the first immediate of each row of the frame buffer.
// Immutable, but in zp.data so it's automatically loaded.
__attribute__((section(".zp.data"))) volatile u8 *framebuffer_row_ptrs[FRAMEBUFFER_HEIGHT_TILES] = {
  &framebuffer[1 + 0 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 1 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 2 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 3 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 4 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 5 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 6 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 7 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 8 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 9 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 10 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 11 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 12 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 13 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 14 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 15 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 16 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 17 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 18 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 19 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
  &framebuffer[1 + 20 * framebuffer_stride * FRAMEBUFFER_WIDTH_TILES],
};

// Two columns of the frame buffer, expanded out to one byte per pixel.
u8 framebuffer_columns[2][FRAMEBUFFER_HEIGHT];