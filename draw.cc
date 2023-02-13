#include "draw.h"

#include "screen.h"

__attribute__((no_builtin("memset"))) void clear_screen() {
  for (int i = 0; i < 256; i++) {
    fb_next[i] = 0;
    (fb_next + 256)[i] = 0;
    (fb_next + 512)[i] = 0;
  }
  for (uint8_t i = 0; i < 192; i++)
    (fb_next + 768)[i] = 0;
}

#if 0
extern "C" {
void draw_solid_column_even(uint8_t *col, uint8_t color, uint8_t y_top,
                            uint8_t y_bot) {
  if (y_bot == y_top)
    return;
  uint8_t i = y_top / 2;
  if (y_top & 1) {
    col[i] &= 0b11110011;
    col[i] |= color << 2;
    i++;
  }
  for (; i < y_bot / 2; i++) {
    col[i] &= 0b11110000;
    col[i] |= color << 2 | color;
  }
  if (y_bot & 1) {
    col[i] &= 0b11111100;
    col[i] |= color;
    i++;
  }
}

void draw_solid_column_odd(uint8_t *col, uint8_t color, uint8_t y_top,
                           uint8_t y_bot) {
  if (y_bot == y_top)
    return;
  uint8_t i = y_top / 2;
  if (y_top & 1) {
    col[i] &= 0b00111111;
    col[i] |= color << 6;
    i++;
  }
  for (; i < y_bot / 2; i++) {
    col[i] &= 0b00001111;
    col[i] |= color << 6 | color << 4;
  }
  if (y_bot & 1) {
    col[i] &= 0b11001111;
    col[i] |= color << 4;
    i++;
  }
}
}
#endif
