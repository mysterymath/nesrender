#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

void clear_screen();

constexpr uint16_t screen_guard = 5 * 256;

// Values are 8:8 fixed point screen pixels, but offset by screen_guard.
void line_move_to(uint16_t x, uint16_t y);
void line_draw_to(uint8_t color, uint16_t x, uint16_t y);

// Values are actual pixel locations.
void clear_z();
void wall_move_to(uint16_t x, uint16_t y_top, uint16_t y_bot, uint16_t z);
void wall_draw_to(uint8_t color, uint16_t x, uint16_t y_top, uint16_t y_bot,
                  uint16_t z);

#endif // not DRAW_H
