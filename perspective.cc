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
  DEBUG("Begin frame.\n");
  clear_screen();
  clear_z();
  move_to(400, 400);
  draw_to(400, 600);
  draw_to(600, 600);
  draw_to(500, 500);
  draw_to(600, 400);
  draw_to(400, 400);
}

static void xy_to_cc(uint16_t x, uint16_t y, int16_t *cc_x, int16_t *cc_w);
static void z_to_cc(uint16_t z, int16_t *cc_y);
static void to_screen(int16_t cc_x, int16_t cc_y_top, int16_t cc_y_bot,
                      int16_t cc_w, uint16_t *sx, uint16_t *sy_top,
                      uint16_t *sy_bot, uint16_t *sz);
static bool in_frustum(int16_t cc_x, int16_t cc_y_top, int16_t cc_y_bot,
                       int16_t cc_w);
static bool wall_on_screen(int16_t cc_x1, int16_t cc_y_top1, int16_t cc_y_bot1,
                           int16_t cc_w1, int16_t cc_x2, int16_t cc_y_top2,
                           int16_t cc_y_bot2, int16_t cc_w2);

static int16_t cur_cc_x;
static int16_t cur_cc_y_top;
static int16_t cur_cc_y_bot;
static int16_t cur_cc_w;
static bool cur_on_screen;

constexpr int16_t wall_top_z = 80;
constexpr int16_t wall_bot_z = 20;

static void update_cur();

#define DEBUG_CC(PREFIX, NAME)                                                 \
  DEBUG("%s: (%d,[%d,%d],%d)\n", PREFIX, NAME##_x, NAME##_y_top, NAME##_y_bot, \
        NAME##_w)

#define DEBUG_SCREEN(PREFIX)                                                   \
  DEBUG("%s: (%d:%d,[%d:%d,%d:%d],%u)\n", PREFIX, (int16_t)sx / 256 - 5,       \
        sx % 256, (int16_t)sy_top / 256 - 5, sy_top % 256,                     \
        (int16_t)sy_bot / 256 - 5, sy_bot % 256, sz)

static void move_to(uint16_t x, uint16_t y) {
  DEBUG("Move to: (%u,%u)\n", x, y);
  xy_to_cc(x, y, &cur_cc_x, &cur_cc_w);
  z_to_cc(wall_top_z, &cur_cc_y_top);
  z_to_cc(wall_bot_z, &cur_cc_y_bot);
  update_cur();
}

static void update_cur() {
  DEBUG_CC("Update cur", cur_cc);
  DEBUG("\n");
  if (in_frustum(cur_cc_x, cur_cc_y_top, cur_cc_y_bot, cur_cc_w)) {
    DEBUG("In frustum.\n");
    uint16_t sx, sy_top, sy_bot, sz;
    to_screen(cur_cc_x, cur_cc_y_top, cur_cc_y_bot, cur_cc_w, &sx, &sy_top,
              &sy_bot, &sz);

    DEBUG_SCREEN("Screen: ");
    wall_move_to(sx, sy_top, sy_bot, sz);
    cur_on_screen = true;
  } else {
    DEBUG("Not in frustum.\n");
    cur_on_screen = false;
  }
}

static void draw_to_cc(int16_t cc_x, int16_t cc_y_top, int16_t cc_y_bot,
                       int16_t cc_w);

static void draw_to(uint16_t x, uint16_t y) {
  DEBUG("Draw to: (%u,%u)\n", x, y);

  int16_t cc_x, cc_w;
  xy_to_cc(x, y, &cc_x, &cc_w);
  int16_t cc_y_top, cc_y_bot;
  z_to_cc(wall_top_z, &cc_y_top);
  z_to_cc(wall_bot_z, &cc_y_bot);
  DEBUG_CC("Draw to", cc);

  if (!wall_on_screen(cur_cc_x, cur_cc_y_top, cur_cc_y_bot, cur_cc_w, cc_x,
                      cc_y_top, cc_y_bot, cc_w)) {
    cur_cc_x = cc_x;
    cur_cc_y_top = cc_y_top;
    cur_cc_y_bot = cc_y_bot;
    cur_cc_w = cc_w;
    cur_on_screen = false;
    DEBUG("Wall entirely outside frustum. Culled.\n");
    return;
  }

  draw_to_cc(cc_x, cc_y_top, cc_y_bot, cc_w);
  cur_cc_x = cc_x;
  cur_cc_y_top = cc_y_top;
  cur_cc_y_bot = cc_y_bot;
  cur_cc_w = cc_w;
  update_cur();
}

static void draw_clipped(int16_t cc_x, int16_t cc_y_top, int16_t cc_y_bot,
                         int16_t cc_w);

static void draw_to_cc(int16_t cc_x, int16_t cc_y_top, int16_t cc_y_bot,
                       int16_t cc_w) {
  DEBUG_CC("Draw to cc", cc);
  bool on_screen = in_frustum(cc_x, cc_y_top, cc_y_bot, cc_w);
  DEBUG("%s frustum\n", on_screen ? "In" : "Not in");
  if (cur_on_screen && on_screen) {
    uint16_t sx, sy_top, sy_bot, sz;
    to_screen(cc_x, cc_y_top, cc_y_bot, cc_w, &sx, &sy_top, &sy_bot, &sz);
    DEBUG_SCREEN("Screen:");
    wall_draw_to(1, sx, sy_top, sy_bot, sz);
  } else {
    draw_clipped(cc_x, cc_y_top, cc_y_bot, cc_w);
  }

  // Note: This may be clipped, and is set for the benefit of a later draw_to_cc
  // call that is part of the same logical wall, not for the benefit of the next
  // draw_to.
  cur_cc_x = cc_x;
  cur_cc_y_top = cc_y_top;
  cur_cc_y_bot = cc_y_bot;
  cur_cc_w = cc_w;
  cur_on_screen = on_screen;
}

static void draw_clipped(int16_t cc_x, int16_t cc_y_top, int16_t cc_y_bot,
                         int16_t cc_w) {
  int16_t dx = cc_x - cur_cc_x;
  int16_t dy_top = cc_y_top - cur_cc_y_top;
  int16_t dy_bot = cc_y_bot - cur_cc_y_bot;
  int16_t dw = cc_w - cur_cc_w;

  if (cur_cc_x < -cur_cc_w) {
    DEBUG("Wall crosses left frustum edge from left to right. Clipping.\n");
    DEBUG_CC("Cur", cur_cc);
    DEBUG_CC("Next", cc);
    // r(t) = cur + vt
    // w(t)_x = -w(t)_w
    // cur_x + dxt = -cur_w - dw*t;
    // t*(dx + dw) = -cur_w - cur_x
    // t = (-cur_w - cur_x) / (dx + dw)
    // Since the wall cannot be parallel to the left frustum edge, dw + dx !=
    // 0.
    int16_t t_num = -cur_cc_w - cur_cc_x;
    int16_t t_denom = dx + dw;
    cur_cc_x += (int32_t)dx * t_num / t_denom;
    cur_cc_y_top += (int32_t)dy_top * t_num / t_denom;
    cur_cc_y_bot += (int32_t)dy_bot * t_num / t_denom;
    cur_cc_w = -cur_cc_x;
    DEBUG_CC("Clipped Cur", cur_cc);
    update_cur();
  } else if (cc_x < -cc_w) {
    DEBUG("Wall crosses left frustum edge from right to left. Clipping.\n");
    DEBUG_CC("Cur", cur_cc);
    DEBUG_CC("Next", cc);
    int16_t t_num = -cc_w - cc_x;
    int16_t t_denom = dx + dw;
    cc_x += (int32_t)dx * t_num / t_denom;
    cc_y_top += (int32_t)dy_top * t_num / t_denom;
    cc_y_bot += (int32_t)dy_bot * t_num / t_denom;
    cc_w = -cc_x;
    DEBUG_CC("Clipped Next", cc);
  }
  if (cur_cc_x > cur_cc_w) {
    DEBUG("Wall crosses right frustum edge from right to left. Clipping.\n");
    DEBUG_CC("Cur", cur_cc);
    DEBUG_CC("Next", cc);
    // r(t) = cur + vt
    // r(t)_x = r(t)_w
    // cur_x + dx*t = cur_w + dw*t;
    // t*(dx - dw) = cur_w - cur_x
    // t = (cur_w - cur_x) / (dx - dw)
    // Since the wall cannot be parallel to the right frustum edge, dx - dw !=
    // 0.
    int16_t t_num = cur_cc_w - cur_cc_x;
    int16_t t_denom = dx - dw;
    cur_cc_x += (int32_t)dx * t_num / t_denom;
    cur_cc_y_top += (int32_t)dy_top * t_num / t_denom;
    cur_cc_y_bot += (int32_t)dy_bot * t_num / t_denom;
    cur_cc_w = cur_cc_x;
    DEBUG_CC("Clipped Cur", cur_cc);
    update_cur();
  } else if (cc_x > cc_w) {
    DEBUG("Wall crosses right frustum edge from left to right. Clipping.\n");
    DEBUG_CC("Cur", cur_cc);
    DEBUG_CC("Next", cc);
    int16_t t_num = cc_w - cc_x;
    int16_t t_denom = dx - dw;
    cc_x += (int32_t)dx * t_num / t_denom;
    cc_y_top += (int32_t)dy_top * t_num / t_denom;
    cc_y_bot += (int32_t)dy_bot * t_num / t_denom;
    cc_w = cc_x;
    DEBUG_CC("Clipped Next", cc);
  }
  if (cur_cc_y_top > cur_cc_w && cc_y_top > cc_w) {
    DEBUG("Wall left and right edge cross frustum top. Clipping.\n");

    DEBUG_CC("Cur", cur_cc);
    cur_cc_y_top = cur_cc_w;
    DEBUG_CC("Clipped Cur", cur_cc);
    update_cur();

    DEBUG_CC("Next", cc);
    cc_y_top = cc_w;
    DEBUG_CC("Clipped Next", cc);
  } else if (cur_cc_y_top > cur_cc_w || cc_y_top > cc_w) {
    DEBUG("Wall top edge crosses frustum top. Clipping.\n");
    DEBUG_CC("Cur", cur_cc);
    DEBUG_CC("Next", cc);

    // r(t) = cur + vt
    // r(t)_y = r(t)_w
    // cur_y + dyt = cur_w + dwt
    // t = (cur_w - cur_y) / (dy - dw)
    int16_t t_num = cur_cc_w - cur_cc_y_top;
    int16_t t_denom = dy_top - dw;
    int16_t isect_cc_x = cur_cc_x + (int32_t)dx * t_num / t_denom;
    int16_t isect_cc_y_top = cur_cc_y_top + (int32_t)dy_top * t_num / t_denom;
    int16_t isect_cc_y_bot = cur_cc_y_bot + (int32_t)dy_bot * t_num / t_denom;
    int16_t isect_cc_w = isect_cc_y_top;
    DEBUG_CC("Intersection", isect_cc);
    if (cur_cc_y_top > cur_cc_w) {
      cur_cc_y_top = cur_cc_w;
      DEBUG_CC("Clipped Cur:", cur_cc);
      update_cur();
    } else {
      cc_y_top = cc_w;
      DEBUG_CC("Clipped Next:", cc);
    }
    draw_to_cc(isect_cc_x, isect_cc_y_top, isect_cc_y_bot, isect_cc_w);
  }
  if (cur_cc_y_bot < -cur_cc_w && cc_y_bot < -cc_w) {
    DEBUG("Wall left and right edge cross frustum bottom. Clipping.\n");

    DEBUG_CC("Cur", cur_cc);
    cur_cc_y_bot = -cur_cc_w;
    DEBUG_CC("Clipped Cur", cur_cc);
    update_cur();

    DEBUG_CC("Next", cc);
    cc_y_bot = -cc_w;
    DEBUG_CC("Clipped Next", cc);
  } else if (cur_cc_y_bot < -cur_cc_w || cc_y_bot < -cc_w) {
    DEBUG("Wall bottom edge crosses frustum bottom. Clipping.\n");
    DEBUG_CC("Cur", cur_cc);
    DEBUG_CC("Next", cc);

    // r(t) = cur + vt
    // r(t)_y = -r(t)_w
    // cur_y + dyt = -cur_w - dwt
    // t = (-cur_w - cur_y) / (dy + dw)
    int16_t t_num = -cur_cc_w - cur_cc_y_bot;
    int16_t t_denom = dy_bot + dw;
    int16_t isect_cc_x = cur_cc_x + (int32_t)dx * t_num / t_denom;
    int16_t isect_cc_y_top = cur_cc_y_top + (int32_t)dy_top * t_num / t_denom;
    int16_t isect_cc_y_bot = cur_cc_y_bot + (int32_t)dy_bot * t_num / t_denom;
    int16_t isect_cc_w = -isect_cc_y_bot;
    DEBUG_CC("Intersection", isect_cc);

    if (cur_cc_y_bot < -cur_cc_w) {
      cur_cc_y_bot = -cur_cc_w;
      DEBUG_CC("Clipped Cur:", cur_cc);
      update_cur();
    } else {
      cc_y_bot = -cc_w;
      DEBUG_CC("Clipped Next:", cc);
    }
    draw_to_cc(isect_cc_x, isect_cc_y_top, isect_cc_y_bot, isect_cc_w);
  }
  if (in_frustum(cc_x, cc_y_top, cc_y_bot, cc_w)) {
    DEBUG("Next in frustum.\n");
    uint16_t sx, sy_top, sy_bot, sz;
    to_screen(cc_x, cc_y_top, cc_y_bot, cc_w, &sx, &sy_top, &sy_bot, &sz);
    DEBUG_SCREEN("Screen");
    if (cur_on_screen)
      wall_draw_to(1, sx, sy_top, sy_bot, sz);
    else
      wall_move_to(sx, sy_top, sy_bot, sz);
  }
}

static void xy_to_cc(uint16_t x, uint16_t y, int16_t *cc_x, int16_t *cc_w) {
  int16_t tx = x - player.x;
  int16_t ty = y - player.y;
  int16_t vx = mul_cos(-player.ang, tx) - mul_sin(-player.ang, ty);
  int16_t vy = mul_sin(-player.ang, tx) + mul_cos(-player.ang, ty);
  *cc_x = -vy;
  *cc_w = vx;
}

static void z_to_cc(uint16_t z, int16_t *cc_y) {
  int16_t vz = z - player.z;
  // The frustum's top world coordinate is z = kx, for some k.
  // The frustum's left world coordinate is y = x.
  // When the frustum is 2w wide, y = x = w. Then the frustum must be 2h tall,
  // and z = kx = h. So kw = h, and k = h/w.
  // The line z=h/wx should map to (1,1), so scale by the inverse factor.
  *cc_y = vz * (int32_t)screen_width / screen_height;
}

static void to_screen(int16_t cc_x, int16_t cc_y_top, int16_t cc_y_bot,
                      int16_t cc_w, uint16_t *sx, uint16_t *sy_top,
                      uint16_t *sy_bot, uint16_t *sz) {
  *sx = (int32_t)cc_x * (screen_width / 2) * 256 / cc_w +
        screen_width / 2 * 256 + screen_guard;
  *sy_top = (int32_t)-cc_y_top * (screen_height / 2) * 256 / cc_w +
            screen_height / 2 * 256 + screen_guard;
  *sy_bot = (int32_t)-cc_y_bot * (screen_height / 2) * 256 / cc_w +
            screen_height / 2 * 256 + screen_guard;
  *sz = (int32_t)-65536 / cc_w;
}

static constexpr int16_t frustum_guard = 1;

static bool in_frustum(int16_t cc_x, int16_t cc_y_top, int16_t cc_y_bot,
                       int16_t cc_w) {
  if (cc_w <= 0)
    return false;
  // Using a FOV of 90 degrees, the frustum left and right are defined by y =
  // +-w.
  if (abs(cc_x) > cc_w + frustum_guard)
    return false;

  if (abs(cc_y_top) > cc_w + frustum_guard)
    return false;
  return abs(cc_y_bot) <= cc_w + frustum_guard;
}

static bool wall_on_screen(int16_t cc_x1, int16_t cc_y_top1, int16_t cc_y_bot1,
                           int16_t cc_w1, int16_t cc_x2, int16_t cc_y_top2,
                           int16_t cc_y_bot2, int16_t cc_w2) {
  if (cc_x1 < -cc_w1 && cc_x2 < -cc_w2) {
    DEBUG("Both points to the left of left frustum edge.\n");
    return false;
  }
  if (cc_x1 > cc_w1 && cc_x2 > cc_w2) {
    DEBUG("Both points to the right of right frustum edge.\n");
    return false;
  }
  if (cc_y_bot1 > cc_w1 && cc_y_bot2 > cc_w2) {
    DEBUG("Bottom of both walls above top frustum edge.\n");
    return false;
  }
  if (cc_y_top1 < -cc_w1 && cc_y_top2 < -cc_w2) {
    DEBUG("Top of both walls below bottom frustum edge.\n");
    return false;
  }
  return true;
}
