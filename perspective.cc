#include "perspective.h"

#include <stdint.h>

#include "draw.h"
#include "map.h"
#include "screen.h"
#include "util.h"

static void move_to(uint16_t x, uint16_t y);
static void draw_to(uint16_t x, uint16_t y);

void perspective::render() {
  clear_screen();
  move_to(100, 100);
  draw_to(100, 200);
  draw_to(200, 200);
  draw_to(200, 100);
  draw_to(100, 100);
}

static int16_t cur_vc_x;
static int16_t cur_vc_y;

static bool in_frustum(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                       int16_t vc_z_bot);
static void to_screen(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                      int16_t vc_z_bot, uint8_t *sx, uint8_t *sy_top,
                      uint8_t *sy_bot);

constexpr int16_t wall_top_z = 100;
constexpr int16_t wall_bot_z = 0;

static void move_to(uint16_t x, uint16_t y) {
  player.to_vc(x, y, &cur_vc_x, &cur_vc_y);
  int16_t vc_z_top, vc_z_bot;
  player.to_vc_z(wall_top_z, &vc_z_top);
  player.to_vc_z(wall_bot_z, &vc_z_bot);
  if (in_frustum(cur_vc_x, cur_vc_y, vc_z_top, vc_z_bot)) {
    uint8_t sx, sy_top, sy_bot;
    to_screen(cur_vc_x, cur_vc_y, vc_z_top, vc_z_bot, &sx, &sy_top, &sy_bot);
    wall_move_to(sx, sy_top, sy_bot);
  }
}

static void draw_to(uint16_t x, uint16_t y) {}

static void to_screen(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                      int16_t vc_z_bot, uint8_t *sx, uint8_t *sy_top,
                      uint8_t *sy_bot) {}

static bool in_frustum(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                       int16_t vc_z_bot) {
  // TODO: Z
  if (vc_x <= 0)
    return false;
  // Using a FOV of 90 degrees, the frustum left and right are defined by y =
  // +-x.
  if (abs(vc_y) > vc_x)
    return false;

  // Since the frustum left and right are y = +-x, take y = x = w/2. This should
  // lead to a world rectangle of width w and height h, so z must be h/2. But z
  // is proportional to x, so the frustum top and bottom must be z =
  // +-(h/2)x/(w/2) = +- hx/w. So, z is in frustum iff abs(z) <= hx/w, or
  // abs(z)*w <= hx.

  // BUT, this doesn't work! The top could be in frustum, but the bottom isn't,
  // or vice versa!
  return true;
}
