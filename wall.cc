#include "wall.h"

#include "screen.h"

static uint8_t cur_x;
static uint8_t cur_y_top;
static uint8_t cur_y_bot;

void wall_move_to(uint8_t x, uint8_t y_top, uint8_t y_bot) {
  cur_x = x;
  cur_y_top = y_top;
  cur_y_bot = y_bot;
}

template <bool x_odd>
void draw_column(uint8_t color, uint8_t *col, uint8_t y_top, uint8_t y_bot);

void wall_draw_to(uint8_t color, uint8_t x, uint8_t y_top, uint8_t y_bot) {
  uint8_t dx = x - cur_x;
  int8_t dy_top = y_top - cur_y_top;
  int8_t dy_bot = y_bot - cur_y_bot;
  // Values in 8.8 fixed point
  int m_top = dy_top * 256 / dx;
  int m_bot = dy_bot * 256 / dx;

  uint16_t y_top_fp = cur_y_top << 8;
  uint16_t y_bot_fp = cur_y_bot << 8;

  uint8_t *fb_col = &fb_next[cur_x / 2 * 30];
  for (uint8_t draw_x = cur_x; draw_x < x; ++draw_x) {
    if (draw_x & 1) {
      draw_column<true>(3, fb_col, 0, y_top_fp / 256);
      draw_column<true>(color, fb_col, y_top_fp / 256, y_bot_fp / 256);
      draw_column<true>(0, fb_col, y_bot_fp / 256, 61);
      fb_col += 30;
    } else {
      draw_column<false>(3, fb_col, 0, y_top_fp / 256);
      draw_column<false>(color, fb_col, y_top_fp / 256, y_bot_fp / 256);
      draw_column<false>(0, fb_col, y_bot_fp / 256, 61);
    }
    y_top_fp += m_top;
    y_bot_fp += m_bot;
  }
  cur_x = x;
  cur_y_top = y_top;
  cur_y_bot = y_bot;
}

template <bool x_odd>
void draw_column(uint8_t color, uint8_t *col, uint8_t y_top, uint8_t y_bot) {
  if (y_top >= y_bot)
    return;
  uint8_t i = y_top / 2;
  if (y_top & 1) {
    if (x_odd) {
      col[i] &= 0b00111111;
      col[i] |= color << 6;
    } else {
      col[i] &= 0b11110011;
      col[i] |= color << 2;
    }
    i++;
  }
  while (i < y_bot / 2) {
    if (x_odd) {
      col[i] &= 0b00001111;
      col[i] |= color << 6 | color << 4;
    } else {
      col[i] &= 0b11110000;
      col[i] |= color << 2 | color << 0;
    }
    i++;
  }
  if (y_bot & 1) {
    if (x_odd) {
      col[i] &= 0b11001111;
      col[i] |= color << 4;
    } else {
      col[i] &= 0b11111100;
      col[i] |= color << 0;
    }
  }
}
