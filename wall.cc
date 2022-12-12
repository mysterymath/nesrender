#include "draw.h"

#include <stdio.h>

#include "screen.h"
#include "util.h"

// #define DEBUG_FILE
#include "debug.h"

static uint16_t cur_x;
static uint16_t cur_y_top;
static uint16_t cur_y_bot;

void wall_move_to(uint16_t x, uint16_t y_top, uint16_t y_bot) {
  cur_x = x;
  cur_y_top = y_top;
  cur_y_bot = y_bot;
}

template <bool x_odd>
void draw_column(uint8_t color, uint8_t *col, uint8_t y_top, uint8_t y_bot);

void wall_draw_to(uint8_t color, uint16_t to_x, uint16_t to_y_top,
                  uint16_t to_y_bot) {
  DEBUG("Draw wall from (%u,%u-%u) to (%u,%u-%u)\n", cur_x, cur_y_top,
        cur_y_bot, to_x, to_y_top, to_y_bot);
  int16_t dx = to_x - cur_x;
  if (!dx)
    return;
  int16_t dy_top = to_y_top - cur_y_top;
  int16_t dy_bot = to_y_bot - cur_y_bot;
  // Values in 8.8 fixed point
  int16_t m_top = (int32_t)dy_top * 256 / abs(dx);
  int16_t m_bot = (int32_t)dy_bot * 256 / abs(dx);
  dx = dx < 0 ? -256 : 256;

  DEBUG("dx: %d, m_top: %d, m_bot: %d\n", dx, m_top, m_bot);

  uint16_t x = cur_x;
  uint16_t y_top = cur_y_top;
  uint16_t y_bot = cur_y_bot;

  while (x / 256 != to_x / 256 &&
         (x < screen_guard || x >= screen_width * 256 + screen_guard)) {
    y_top += m_top;
    y_bot += m_bot;
    x += dx;
  }

  x -= screen_guard;
  to_x -= screen_guard;

  uint8_t *fb_col = &fb_next[x / 256 / 2 * 30];
  const auto y_pix = [&](int16_t y) -> uint8_t {
    if (y < screen_guard)
      return 0;
    if (y > screen_height * 256 + screen_guard)
      return screen_height;
    return (y - screen_guard) / 256;
  };
  while (x / 256 != to_x / 256) {
    uint8_t x_pix = x / 256;
    uint8_t y_top_pix = y_pix(y_top);
    uint8_t y_bot_pix = y_pix(y_bot);
    if (x_pix & 1) {
      draw_column<true>(3, fb_col, 0, y_top_pix);
      draw_column<true>(color, fb_col, y_top_pix, y_bot_pix);
      draw_column<true>(0, fb_col, y_bot_pix, screen_height);
      fb_col += 30;
    } else {
      draw_column<false>(3, fb_col, 0, y_top_pix);
      draw_column<false>(color, fb_col, y_top_pix, y_bot_pix);
      draw_column<false>(0, fb_col, y_bot_pix, screen_height);
    }
    y_top += m_top;
    y_bot += m_bot;
    x += dx;
  }
  cur_x = x + screen_guard;
  cur_y_top = y_top;
  cur_y_bot = y_bot;
}

// Note: y_bot is exclusive.
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
