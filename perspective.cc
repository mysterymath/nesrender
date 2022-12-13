#include "perspective.h"

#include <stdint.h>
#include <stdio.h>

#include "draw.h"
#include "map.h"
#include "screen.h"
#include "trig.h"
#include "util.h"

#pragma clang section text = ".prg_rom_0.text" rodata = ".prg_rom_0.rodata"

//#define DEBUG_FILE
#include "debug.h"

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
static int16_t cur_vc_z_top;
static int16_t cur_vc_z_bot;
static bool cur_on_screen;

static bool in_frustum(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                       int16_t vc_z_bot);
static bool z_in_frustum(int16_t vc_x, int16_t vc_z);
static bool wall_on_screen(int16_t vc_x1, int16_t vc_y1, int16_t vc_z_top1,
                           int16_t vc_z_bot1, int16_t vc_x2, int16_t vc_y2,
                           int16_t vc_z_top2, int16_t vc_z_bot2);
static void draw_clipped(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                         int16_t vc_z_bot);
static void to_screen(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                      int16_t vc_z_bot, uint16_t *sx, uint16_t *sy_top,
                      uint16_t *sy_bot);
static void to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y);
static void to_vc_z(uint16_t z, int16_t *vc_z);

constexpr int16_t wall_top_z = 60;
constexpr int16_t wall_bot_z = 40;

static void move_to(uint16_t x, uint16_t y) {
  DEBUG("Move to: %u, %u\n", x, y);
  to_vc(x, y, &cur_vc_x, &cur_vc_y);
  to_vc_z(wall_top_z, &cur_vc_z_top);
  to_vc_z(wall_bot_z, &cur_vc_z_bot);
  DEBUG("vc_x: %d, y: %d, z_top: %d, z_bot: %d\n", cur_vc_x, cur_vc_y,
        cur_vc_z_top, cur_vc_z_bot);
  if (in_frustum(cur_vc_x, cur_vc_y, cur_vc_z_top, cur_vc_z_bot)) {
    DEBUG("in frustum.\n");
    uint16_t sx, sy_top, sy_bot;
    to_screen(cur_vc_x, cur_vc_y, cur_vc_z_top, cur_vc_z_bot, &sx, &sy_top,
              &sy_bot);
    DEBUG("sx: %u, y_top: %u, y_bot: %u\n", sx, sy_top, sy_bot);
    wall_move_to(sx, sy_top, sy_bot);
    cur_on_screen = true;
  } else {
    DEBUG("not in frustum.\n");
    cur_on_screen = false;
  }
}

static void draw_to(uint16_t x, uint16_t y) {
  DEBUG("Draw to: %u, %u\n", x, y);

  int16_t vc_x, vc_y;
  to_vc(x, y, &vc_x, &vc_y);
  int16_t vc_z_top, vc_z_bot;
  to_vc_z(wall_top_z, &vc_z_top);
  to_vc_z(wall_bot_z, &vc_z_bot);
  DEBUG("vc_x: %d, y: %d, z_top: %d, z_bot: %d\n", vc_x, vc_y, vc_z_top,
        vc_z_bot);

  if (!wall_on_screen(cur_vc_x, cur_vc_y, cur_vc_z_top, cur_vc_z_bot, vc_x,
                      vc_y, vc_z_top, vc_z_bot)) {
    cur_vc_x = vc_x;
    cur_vc_y = vc_y;
    cur_vc_z_top = vc_z_top;
    cur_vc_z_bot = vc_z_bot;
    cur_on_screen = false;
    DEBUG("Wall entirely outside frustum. Culled.\n");
    return;
  }

  bool on_screen = in_frustum(vc_x, vc_y, vc_z_top, vc_z_bot);
  DEBUG("in frustum: %d\n", on_screen);
  if (cur_on_screen && on_screen) {
    uint16_t sx, sy_top, sy_bot;
    to_screen(vc_x, vc_y, vc_z_top, vc_z_bot, &sx, &sy_top, &sy_bot);
    DEBUG("sx: %u, y_top: %u, y_bot: %u\n", sx, sy_top, sy_bot);
    wall_draw_to(1, sx, sy_top, sy_bot);
  } else {
    draw_clipped(vc_x, vc_y, vc_z_top, vc_z_bot);
  }
  cur_vc_x = vc_x;
  cur_vc_y = vc_y;
  cur_vc_z_top = vc_z_top;
  cur_vc_z_bot = vc_z_bot;
  cur_on_screen = on_screen;
}

static void draw_clipped(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                         int16_t vc_z_bot) {
  int16_t dx = vc_x - cur_vc_x;
  int16_t dy = vc_y - cur_vc_y;
  if (cur_vc_y > cur_vc_x && vc_y <= vc_x) {
    DEBUG("Wall crosses left frustum edge from left to right. Clipping.\n");
    DEBUG("Prev: (%d,%d) to (%d, %d)\n", cur_vc_x, cur_vc_y, vc_x, vc_y);
    // w(t) = cur + (dw)t
    // w(t)_x = w(t)_y
    // cur_x + dx*t = cur_y + dy*t;
    // t*(dx - dy) = cur_y - cur_x
    // t = (cur_y - cur_x) / (dx - dy)
    // Since the wall cannot be parallel to the left frustum edge, dx - dy !=
    // 0.
    int16_t t_num = cur_vc_y - cur_vc_x;
    int16_t t_denom = dx - dy;
    cur_vc_x += (int32_t)dx * t_num / t_denom;
    cur_vc_y += (int32_t)dy * t_num / t_denom;
    DEBUG("Clipped: (%d,%d) to (%d, %d)\n", cur_vc_x, cur_vc_y, vc_x, vc_y);
  } else if (cur_vc_y < -cur_vc_x && vc_y >= -vc_x) {
    DEBUG("Wall crosses right frustum edge from right to left. Backface "
          "culling.\n");
  }
  if (vc_y > vc_x && cur_vc_y <= cur_vc_x) {
    DEBUG("Wall crosses left frustum edge from right to left. Backface "
          "culling.\n");
  } else if (vc_y < -vc_x && cur_vc_y >= -cur_vc_x) {
    DEBUG("Wall crosses right frustum edge from left to right. Clipping.\n");
    DEBUG("Prev: (%d,%d) to (%d, %d)\n", cur_vc_x, cur_vc_y, vc_x, vc_y);
    // w(t) = next + (dw)t
    // w(t)_y = -w(t)_x
    // next_y + dy*t = -next_x - dx*t;
    // t*(dx + dy) = -next_y - next_x
    // t = (-next_y - next_x) / (dx + dy)
    // Since the wall cannot be parallel to the right frustum edge, dx + dy !=
    // 0.
    int16_t t_num = -vc_y - vc_x;
    int16_t t_denom = dx + dy;
    vc_x += (int32_t)dx * t_num / t_denom;
    vc_y += (int32_t)dy * t_num / t_denom;
    DEBUG("Clipped: (%d,%d) to (%d, %d)\n", cur_vc_x, cur_vc_y, vc_x, vc_y);
  }
  if (cur_vc_z_top * (int32_t)screen_width >=
          cur_vc_x * (int32_t)screen_height &&
      vc_z_top * (int32_t)screen_width <
          vc_x * (int32_t)screen_height) {
    DEBUG("Wall crosses top frustum edge with left side higher from camera "
          "POV. Clipping.\n");
  }
  bool cur_in_frustum =
      in_frustum(cur_vc_x, cur_vc_y, cur_vc_z_top, cur_vc_z_bot);
  if (cur_in_frustum) {
    DEBUG("Cur in frustum.\n");
    uint16_t sx, sy_top, sy_bot;
    to_screen(cur_vc_x, cur_vc_y, cur_vc_z_top, vc_z_bot, &sx, &sy_top,
              &sy_bot);
    DEBUG("sx: %u, y_top: %u, y_bot: %u\n", sx, sy_top, sy_bot);
    wall_move_to(sx, sy_top, sy_bot);
  }
  if (in_frustum(vc_x, vc_y, vc_z_top, vc_z_bot)) {
    DEBUG("Next in frustum.\n");
    uint16_t sx, sy_top, sy_bot;
    to_screen(vc_x, vc_y, vc_z_top, vc_z_bot, &sx, &sy_top, &sy_bot);
    DEBUG("sx: %u, y_top: %u, y_bot: %u\n", sx, sy_top, sy_bot);
    if (cur_in_frustum)
      wall_draw_to(1, sx, sy_top, sy_bot);
    else
      wall_move_to(sx, sy_top, sy_bot);
  }
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
  // [x_h, y_h, w_h] = [-y, -z, x].
  // Then, performing a perspective divide:
  // [x_p, y_p] = [-y / x, -z / x].
  // But, this maps all points along the left/right frustum to [+-1, +-1].
  // Accordingly, we need to scale by width/2, then add width/2 or height/2.

  *sx = (int32_t)-vc_y * (screen_width / 2) * 256 / vc_x +
        screen_width / 2 * 256 + screen_guard;
  *sy_top = (int32_t)-vc_z_top * (screen_width / 2) * 256 / vc_x +
            screen_height / 2 * 256 + screen_guard;
  *sy_bot = (int32_t)-vc_z_bot * (screen_width / 2) * 256 / vc_x +
            screen_height / 2 * 256 + screen_guard;
}

static constexpr int16_t frustum_guard = 1;

static bool in_frustum(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                       int16_t vc_z_bot) {
  if (vc_x <= -frustum_guard)
    return false;
  // Using a FOV of 90 degrees, the frustum left and right are defined by y =
  // +-x.
  if (abs(vc_y) > vc_x + frustum_guard)
    return false;

  return z_in_frustum(vc_x, vc_z_top) && z_in_frustum(vc_x, vc_z_bot);
}

static bool z_in_frustum(int16_t vc_x, int16_t vc_z) {
  // Since the frustum left and right are y = +-x, take y = x = w/2. This should
  // lead to a world rectangle of width w and height h, so z must be h/2. But z
  // is proportional to x, so the frustum top and bottom must be z =
  // +-(h/2)x/(w/2) = +- hx/w. So, z is in frustum iff abs(z) <= hx/w, that is,
  // abs(z)*w <= hx.
  return abs(vc_z) * (int32_t)screen_width <=
         (vc_x + frustum_guard) * (int32_t)screen_height;
}

static bool wall_on_screen(int16_t vc_x1, int16_t vc_y1, int16_t vc_z_top1,
                           int16_t vc_z_bot1, int16_t vc_x2, int16_t vc_y2,
                           int16_t vc_z_top2, int16_t vc_z_bot2) {
  if (vc_y1 > vc_x1 && vc_y2 > vc_x2) {
    DEBUG("Both points to the left of left frustum edge.\n");
    return false;
  }
  if (vc_y1 < -vc_x1 && vc_y2 < -vc_x2) {
    DEBUG("Both points to the right of right frustum edge.\n");
    return false;
  }
  if (vc_z_bot1 * (int32_t)screen_width > vc_x1 * (int32_t)screen_height &&
      vc_z_bot2 * (int32_t)screen_width > vc_x2 * (int32_t)screen_height) {
    DEBUG("Bottom of both walls above top frustum edge.\n");
    return false;
  }
  if (vc_z_top1 * (int32_t)screen_width < -vc_x1 * (int32_t)screen_height &&
      vc_z_top2 * (int32_t)screen_width < -vc_x2 * (int32_t)screen_height) {
    DEBUG("Top of both walls below bottom frustum edge.\n");
    return false;
  }
  return true;
}
