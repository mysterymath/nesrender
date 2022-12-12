#include "perspective.h"

#include <stdint.h>
#include <stdio.h>

#include "draw.h"
#include "map.h"
#include "screen.h"
#include "trig.h"
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
static bool cur_on_screen;

static bool in_frustum(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                       int16_t vc_z_bot);
static void to_screen(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                      int16_t vc_z_bot, uint16_t *sx, uint16_t *sy_top,
                      uint16_t *sy_bot);
static void to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y);
static void to_vc_z(uint16_t z, int16_t *vc_z);

constexpr int16_t wall_top_z = 60;
constexpr int16_t wall_bot_z = 40;

static void move_to(uint16_t x, uint16_t y) {
  printf("Move to: %u, %u\n", x, y);
  to_vc(x, y, &cur_vc_x, &cur_vc_y);
  int16_t vc_z_top, vc_z_bot;
  to_vc_z(wall_top_z, &vc_z_top);
  to_vc_z(wall_bot_z, &vc_z_bot);
  printf("vc_x: %d, y: %d, z_top: %d, z_bot: %d\n", cur_vc_x, cur_vc_y,
         vc_z_top, vc_z_bot);
  if (in_frustum(cur_vc_x, cur_vc_y, vc_z_top, vc_z_bot)) {
    printf("in frustum.\n");
    uint16_t sx, sy_top, sy_bot;
    to_screen(cur_vc_x, cur_vc_y, vc_z_top, vc_z_bot, &sx, &sy_top, &sy_bot);
    wall_move_to(sx, sy_top, sy_bot);
    cur_on_screen = true;
  } else {
    printf("not in frustum.\n");
    cur_on_screen = false;
  }
}

static void draw_to(uint16_t x, uint16_t y) {
  printf("Draw to: %u, %u\n", x, y);

  to_vc(x, y, &cur_vc_x, &cur_vc_y);
  int16_t vc_z_top, vc_z_bot;
  to_vc_z(wall_top_z, &vc_z_top);
  to_vc_z(wall_bot_z, &vc_z_bot);

  printf("vc_x: %d, y: %d, z_top: %d, z_bot: %d\n", cur_vc_x, cur_vc_y,
         vc_z_top, vc_z_bot);

  bool on_screen = in_frustum(cur_vc_x, cur_vc_y, vc_z_top, vc_z_bot);
  printf("in frustum: %d\n", on_screen);
  if (!cur_on_screen || !on_screen) {
    cur_on_screen = on_screen;
    return;
  }

  uint16_t sx, sy_top, sy_bot;
  to_screen(cur_vc_x, cur_vc_y, vc_z_top, vc_z_bot, &sx, &sy_top, &sy_bot);
  wall_draw_to(1, sx, sy_top, sy_bot);
  cur_on_screen = on_screen;
}

static void to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y) {
  int16_t tx = x - player.x;
  int16_t ty = y - player.y;
  *vc_x = mul_cos(-player.ang, tx) - mul_sin(-player.ang, ty);
  *vc_y = mul_sin(-player.ang, tx) + mul_cos(-player.ang, ty);
}

static void to_vc_z(uint16_t z, int16_t *vc_z) { *vc_z = z - player.z; }

static void to_screen(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                      int16_t vc_z_bot, uint16_t *sx, uint16_t *sy_top,
                      uint16_t *sy_bot) {
  // Moving to homogeneous coordinates:
  // [x_h, y_h, w_h] = [-y, z, x].
  // Then, performing a perspective divide:
  // [x_p, y_p] = [-y / x, z / x].
  // But, this maps all points along the left/right frustum to [+-1, +-1].
  // Accordingly, we need to scale by width/2, then add width/2 or height/2.

  *sx =
      (int32_t)-vc_y * (screen_width / 2) * 256 / vc_x + screen_width / 2 * 256;
  *sy_top =
      (int32_t)vc_z_top * (screen_width / 2) * 256 / vc_x + screen_height / 2;
  *sy_bot =
      (int32_t)vc_z_bot * (screen_width / 2) * 256 / vc_x + screen_height / 2;
}

static bool in_frustum(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                       int16_t vc_z_bot) {
  if (vc_x <= 0)
    return false;
  // Using a FOV of 90 degrees, the frustum left and right are defined by y =
  // +-x.
  if (abs(vc_y) > vc_x)
    return false;

  // Since the frustum left and right are y = +-x, take y = x = w/2. This should
  // lead to a world rectangle of width w and height h, so z must be h/2. But z
  // is proportional to x, so the frustum top and bottom must be z =
  // +-(h/2)x/(w/2) = +- hx/w. So, z is in frustum iff abs(z) <= hx/w, that is,
  // abs(z)*w <= hx.
  if (abs(vc_z_top) * screen_width > vc_x * screen_height)
    return false;
  return abs(vc_z_bot) * screen_width <= vc_x * screen_height;
}
