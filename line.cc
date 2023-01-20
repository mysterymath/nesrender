#include "draw.h"

#include "screen.h"
#include "util.h"
#include "log.h"

static uint16_t cur_x;
static uint16_t cur_y;

void line_move_to(uint16_t x, uint16_t y) {
  cur_x = x;
  cur_y = y;
}

void line_draw_to(uint8_t color, uint16_t to_x, uint16_t to_y) {
  int16_t dx = to_x - cur_x;
  int16_t dy = to_y - cur_y;

  bool x_major = abs(dx) >= abs(dy);
  if (x_major) {
    dy = Log(dy) / Log(dx).abs() * Log::pow2(8);
    if (dx > 0)
      dx = 1 * 256;
    else if (dx < 0)
      dx = -1 * 256;
  } else {
    dx = Log(dx) / Log(dy).abs() * Log::pow2(8);
    if (dy > 0)
      dy = 1 * 256;
    else if (dy < 0)
      dy = -1 * 256;
  }

  // Move onto the screen.
  uint16_t x = cur_x;
  uint16_t y = cur_y;
  while (x < screen_guard || y < screen_guard ||
         x >= screen_width * 256 + screen_guard ||
         y >= screen_height * 256 + screen_guard) {
    if (x_major ? x / 256 == to_x / 256 : y / 256 == to_y / 256) {
      cur_x = to_x;
      cur_y = to_y;
      return;
    }
    x += dx;
    y += dy;
  }

  // Switch to unguarded screen coordinates.
  x -= screen_guard;
  to_x -= screen_guard;
  y -= screen_guard;
  to_y -= screen_guard;

  uint16_t offset = x / 256 / 2 * 30 + y / 256 / 2;
  while (x_major ? x / 256 != to_x / 256 : y / 256 != to_y / 256) {
    uint8_t shift = (x / 256 % 2 * 2 + y / 256 % 2) * 2;
    uint8_t and_mask = rotl((uint8_t)0b11111100, shift);
    uint8_t or_mask = color << shift;
    fb_next[offset] &= and_mask;
    fb_next[offset] |= or_mask;

    if (dx < 0 ? x < -dx : x + dx >= screen_width * 256)
      break;
    if (dy < 0 ? y < -dy : y + dy >= screen_height * 256)
      break;

    if ((x + dx) / 256 / 2 > x / 256 / 2)
      offset += 30;
    else if ((x + dx) / 256 / 2 < x / 256 / 2)
      offset -= 30;
    if ((y + dy) / 256 / 2 > y / 256 / 2)
      offset++;
    else if ((y + dy) / 256 / 2 < y / 256 / 2)
      offset--;
    x += dx;
    y += dy;
  }
  cur_x = to_x + screen_guard;
  cur_y = to_y + screen_guard;
}
