#include "perspective.h"

#include <stdint.h>
#include <stdio.h>

#include "draw.h"
#include "log.h"
#include "map.h"
#include "screen.h"
#include "trig.h"
#include "util.h"

#pragma clang section text = ".prg_rom_0.text" rodata = ".prg_rom_0.rodata"

// #define DEBUG_FILE
#include "debug.h"

static void move_to(uint16_t x, uint16_t y);
static void draw_to(uint16_t x, uint16_t y);

void perspective::render() {
  DEBUG("Begin frame.\n");
  clear_screen();
  // move_to(400, 400);
  // draw_to(400, 600);
  // draw_to(600, 600);
  // draw_to(500, 500);
  // draw_to(600, 400);
  // draw_to(400, 400);

  move_to(600, 600);
  draw_to(600, 400);
}

static void xy_to_cc(uint16_t x, uint16_t y, int16_t *cc_x, int16_t *cc_w);
static void z_to_cc(uint16_t z, int16_t *cc_y);

static int16_t cur_cc_x;
static int16_t cur_cc_y_top;
static int16_t cur_cc_y_bot;
static int16_t cur_cc_w;
static Log lcur_cc_x;
static Log lcur_cc_y_top;
static Log lcur_cc_y_bot;
static Log lcur_cc_w;

static bool cur_left_of_left;
static bool cur_right_of_right;

constexpr int16_t wall_top_z = 80;
constexpr int16_t wall_bot_z = 20;

#define DEBUG_CC(PREFIX, NAME)                                                 \
  DEBUG("%s: (%d,[%d,%d],%d)\n", PREFIX, NAME##_x, NAME##_y_top, NAME##_y_bot, \
        NAME##_w)

// 64/60
constexpr Log lw_over_h(false, 191);

// 60/2*256
constexpr Log lh_over_2_times_256(false, 26433);

static void move_to(uint16_t x, uint16_t y) {
  DEBUG("Move to: (%u,%u)\n", x, y);
  xy_to_cc(x, y, &cur_cc_x, &cur_cc_w);
  z_to_cc(wall_top_z, &cur_cc_y_top);
  z_to_cc(wall_bot_z, &cur_cc_y_bot);
  DEBUG_CC("Moved to CC:", cur_cc);

  lcur_cc_x = cur_cc_x;
  lcur_cc_y_bot = cur_cc_y_bot;
  lcur_cc_y_top = cur_cc_y_top;
  lcur_cc_w = cur_cc_w;

  cur_left_of_left = cur_cc_x < -cur_cc_w;
  cur_right_of_right = cur_cc_x > cur_cc_w;
}

extern "C" {
void draw_column_even(uint8_t ceil_color, uint8_t wall_color,
                      uint8_t floor_color, uint8_t *col, uint8_t y_top,
                      uint8_t y_bot);
void draw_column_odd(uint8_t ceil_color, uint8_t wall_color,
                     uint8_t floor_color, uint8_t *col, uint8_t y_top,
                     uint8_t y_bot);
}

static void clip_bot_and_draw(Log lcur_sx, uint16_t cur_sx, uint16_t cur_sy_top,
                              Log lm_top, int16_t m_top, Log lcur_sy_bot,
                              Log lsx, uint16_t sx, Log lsy_bot,
                              bool cur_bot_below_bot, bool bot_below_bot,
                              bool cur_bot_above_top, bool bot_above_top);

__attribute__((noinline)) static void draw_to(uint16_t x, uint16_t y) {
  DEBUG("Draw to: (%u,%u)\n", x, y);

  int16_t cc_x, cc_w;
  xy_to_cc(x, y, &cc_x, &cc_w);

  int16_t cc_y_top, cc_y_bot;
  z_to_cc(wall_top_z, &cc_y_top);
  z_to_cc(wall_bot_z, &cc_y_bot);
  DEBUG_CC("Draw to", cc);

  int16_t orig_cc_x = cc_x;
  int16_t orig_cc_y_top = cc_y_top;
  int16_t orig_cc_y_bot = cc_y_bot;
  int16_t orig_cc_w = cc_w;

  Log lcc_x = orig_cc_x;
  Log lcc_y_top = cc_y_top;
  Log lcc_y_bot = cc_y_bot;
  Log lcc_w = orig_cc_w;

  Log lorig_cc_x = lcc_x;
  Log lorig_cc_y_top = lcc_y_top;
  Log lorig_cc_y_bot = lcc_y_bot;
  Log lorig_cc_w = lcc_w;

  bool left_of_left = cc_x < -cc_w;
  bool right_of_right = cc_x > cc_w;

  // There is homogeneous weirdness when both w are zero. Disallow.
  if (cc_w <= 0 && cur_cc_w <= 0)
    goto done;

  {
    if (cur_left_of_left && left_of_left ||
        cur_right_of_right && right_of_right) {
      DEBUG("LR frustum cull: %d %d\n", cur_left_of_left && left_of_left,
            cur_right_of_right && right_of_right);
      goto done;
    }

    // Use cross product to backface cull.
    if (lcur_cc_x * lcc_w >= lcur_cc_w * lcc_x) {
      DEBUG("Backface cull.\n");
      goto done;
    }

    // Note that the logarithmic sx values range from [-1, 1], while the
    // unsigned values range from [0, screen_width*256].

    Log lcur_sx = lcur_cc_x / lcur_cc_w;
    Log lsx = lcc_x / lcc_w;
    uint16_t cur_sx;
    uint16_t sx;
    if (cur_left_of_left || right_of_right) {
      int16_t dx = cc_x - cur_cc_x;
      int16_t dy_top = cc_y_top - cur_cc_y_top;
      int16_t dy_bot = cc_y_bot - cur_cc_y_bot;
      int16_t dw = cc_w - cur_cc_w;
      Log ldx = dx;
      Log ldy_top = dy_top;
      Log ldy_bot = dy_bot;
      Log ldw = dw;

      if (cur_left_of_left) {
        DEBUG("Wall crosses left frustum edge. Clipping.\n");
        DEBUG_CC("Cur", cur_cc);
        DEBUG_CC("Next", cc);
        // r(t) = cur + vt
        // w(t)_x = -w(t)_w
        // cur_x + dxt = -cur_w - dw*t;
        // t*(dx + dw) = -cur_w - cur_x
        // t = (-cur_w - cur_x) / (dx + dw)
        // Since the wall cannot be parallel to the left frustum edge, dw + dx
        // != 0.
        Log t = Log(-cur_cc_w - cur_cc_x) / Log(dx + dw);
        cur_cc_x += ldx * t;
        lcur_cc_x = cur_cc_x;
        cur_cc_y_top += ldy_top * t;
        cur_cc_y_bot += ldy_bot * t;
        cur_cc_w = -cur_cc_x;
        lcur_cc_w = cur_cc_w;
        lcur_sx = -Log::one();
        cur_sx = 0;
        DEBUG_CC("Clipped Cur", cur_cc);
      } else {
        lcur_sx = lcur_cc_x / lcur_cc_w;
        cur_sx = lcur_sx * Log::pow2(13) + screen_width / 2 * 256;
      }
      if (right_of_right) {
        DEBUG("Wall crosses right frustum edge. Clipping.\n");
        DEBUG_CC("Cur", cur_cc);
        DEBUG_CC("Next", cc);
        Log t = Log(cc_w - cc_x) / Log(dx - dw);
        cc_x += ldx * t;
        lcc_x = cc_x;
        cc_y_top += ldy_top * t;
        lcc_y_top = cc_y_top;
        cc_y_bot += ldy_bot * t;
        lcc_y_bot = cc_y_bot;
        cc_w = cc_x;
        lcc_w = cc_w;
        lsx = Log::one();
        sx = screen_width * 256;
        DEBUG_CC("Clipped Next", cc);
      } else {
        lsx = lcc_x / lcc_w;
        sx = lsx * Log::pow2(13) + screen_width / 2 * 256;
      }
    } else {
      cur_sx = lcur_sx * Log::pow2(13) + screen_width / 2 * 256;
      sx = lsx * Log::pow2(13) + screen_width / 2 * 256;
    }

    bool cur_bot_above_top = cur_cc_y_bot < -cur_cc_w;
    bool cur_top_below_bot = cur_cc_y_top > cur_cc_w;
    bool cur_top_above_top = cur_cc_y_top < -cur_cc_w;
    bool cur_bot_below_bot = cur_cc_y_bot > cur_cc_w;
    bool bot_above_top = cc_y_bot < -cc_w;
    bool top_below_bot = cc_y_top > cc_w;
    bool top_above_top = cc_y_top < -cc_w;
    bool bot_below_bot = cc_y_bot > cc_w;

    // Note that lsy ranges from -1 to 1, while sy ranges from 0 to
    // screen_height.

    Log lcur_sy_top = lcur_cc_y_top / lcur_cc_w;
    Log lcur_sy_bot = lcur_cc_y_bot / lcur_cc_w;
    Log lsy_top = lcc_y_top / lcc_w;
    Log lsy_bot = lcc_y_bot / lcc_w;

    if (cur_bot_above_top && bot_above_top ||
        cur_top_below_bot && top_below_bot) {
      DEBUG("TB Frustum Cull: %d %d\n", cur_bot_above_top && bot_above_top,
            cur_top_below_bot && bot_below_top);
      goto done;
    }

    Log lm_top;
    int16_t m_top;
    uint16_t cur_sy_top;
    Log iscale = Log::pow2(13);
    Log m_denom = lsx * iscale - lcur_sx * iscale;
    if (cur_top_above_top && top_above_top) {
      lm_top = Log::zero();
      m_top = 0;
      cur_sy_top = 0;
      cur_top_above_top = top_above_top = false;
      DEBUG("Clipped top: both sides.\n");
      clip_bot_and_draw(lcur_sx, cur_sx, cur_sy_top, lm_top, m_top, lcur_sy_bot,
                        lsx, sx, lsy_bot, cur_bot_below_bot, bot_below_bot,
                        cur_bot_above_top, bot_above_top);
    } else {
      lm_top = Log(lsy_top * iscale - lcur_sy_top * iscale) / m_denom;
      m_top = lm_top * Log::pow2(8);

      if (cur_top_above_top || top_above_top || cur_top_below_bot ||
          top_below_bot) {
        int16_t dx = cc_x - cur_cc_x;
        int16_t dy_top = cc_y_top - cur_cc_y_top;
        int16_t dy_bot = cc_y_bot - cur_cc_y_bot;
        int16_t dw = cc_w - cur_cc_w;
        Log ldx = dx;
        Log ldy_top = dy_top;
        Log ldy_bot = dy_bot;
        Log ldw = dw;

        if (cur_top_above_top || top_above_top) {
          // r(t) = cur + vt
          // r(t)_y = -r(t)_w
          // cur_y + dyt = -cur_w - dw*t;
          // t*(dy + dw) = -cur_w - cur_y
          // t = (-cur_w - cur_y) / (dy + dw)
          Log t = Log(-cur_cc_w - cur_cc_y_top) / Log(dy_top + dw);
          int16_t isect_cc_x = cur_cc_x + ldx * t;
          int16_t isect_cc_y_top = cur_cc_y_top + ldy_top * t;
          int16_t isect_cc_y_bot = cur_cc_y_bot + ldy_bot * t;
          int16_t isect_cc_w = -isect_cc_y_top;
          bool isect_bot_below_bot = isect_cc_y_bot > isect_cc_w;
          bool isect_bot_above_top = isect_cc_y_bot < -isect_cc_w;
          Log lisect_cc_x = isect_cc_x;
          Log lisect_cc_y_bot = isect_cc_y_bot;
          Log lisect_cc_w = isect_cc_w;
          Log lisect_sx = lisect_cc_x / lisect_cc_w;
          Log lisect_sy_bot = lisect_cc_y_bot / lisect_cc_w;
          uint16_t isect_sx =
              lisect_sx * Log::pow2(13) + screen_width / 2 * 256;
          if (cur_top_above_top) {
            clip_bot_and_draw(lcur_sx, cur_sx, 0, Log::zero(), 0, lcur_sy_bot,
                              lisect_sx, isect_sx, lisect_sy_bot,
                              cur_bot_below_bot, isect_bot_below_bot,
                              cur_bot_above_top, isect_bot_above_top);
            clip_bot_and_draw(lisect_sx, isect_sx, 0, lm_top, m_top,
                              lisect_sy_bot, lsx, sx, lsy_bot,
                              isect_bot_below_bot, bot_below_bot,
                              isect_bot_above_top, bot_above_top);
          } else {
            cur_sy_top =
                lcur_sy_top * lh_over_2_times_256 + screen_height / 2 * 256;
            clip_bot_and_draw(lcur_sx, cur_sx, cur_sy_top, lm_top, m_top,
                              lcur_sy_bot, lisect_sx, isect_sx, lisect_sy_bot,
                              cur_bot_below_bot, isect_bot_below_bot,
                              cur_bot_above_top, isect_bot_above_top);
            clip_bot_and_draw(lisect_sx, isect_sx, 0, Log::zero(), 0,
                              lisect_sy_bot, lsx, sx, lsy_bot,
                              isect_bot_below_bot, bot_below_bot,
                              isect_bot_above_top, bot_above_top);
          }
        } else {
          // TODO
        }
      } else {
        cur_sy_top =
            lcur_sy_top * lh_over_2_times_256 + screen_height / 2 * 256;
        clip_bot_and_draw(lcur_sx, cur_sx, cur_sy_top, lm_top, m_top,
                          lcur_sy_bot, lsx, sx, lsy_bot, cur_bot_below_bot,
                          bot_below_bot, cur_bot_above_top, bot_above_top);
      }
    }
  }

done:
  cur_cc_x = orig_cc_x;
  cur_cc_y_top = orig_cc_y_top;
  cur_cc_y_bot = orig_cc_y_bot;
  cur_cc_w = orig_cc_w;
  lcur_cc_x = lorig_cc_x;
  lcur_cc_y_top = lorig_cc_y_top;
  lcur_cc_y_bot = lorig_cc_y_bot;
  lcur_cc_w = lorig_cc_w;
  cur_left_of_left = left_of_left;
  cur_right_of_right = right_of_right;
}

static void draw_after_fully_clipped(uint16_t cur_sx, uint16_t cur_sy_top,
                                     Log lm_top, int16_t m_top,
                                     uint16_t cur_sy_bot, Log lm_bot,
                                     int16_t m_bot, uint16_t sx);

static void clip_bot_and_draw(Log lcur_sx, uint16_t cur_sx, uint16_t cur_sy_top,
                              Log lm_top, int16_t m_top, Log lcur_sy_bot,
                              Log lsx, uint16_t sx, Log lsy_bot,
                              bool cur_bot_below_bot, bool bot_below_bot,
                              bool cur_bot_above_top, bool bot_above_top) {
  Log lm_bot;
  int16_t m_bot;
  uint16_t cur_sy_bot;
  Log iscale = Log::pow2(13);
  Log m_denom = lsx * iscale - lcur_sx * iscale;
  if (cur_bot_below_bot && bot_below_bot) {
    lm_bot = Log::zero();
    m_bot = 0;
    cur_sy_bot = screen_height * 256;
    cur_bot_below_bot = bot_below_bot = false;
    DEBUG("Clipped bot: both sides.\n");
  } else if (cur_bot_below_bot || bot_below_bot) {
    // TODO
  } else if (cur_bot_above_top || bot_above_top) {
    // TODO
  } else {
    lm_bot = Log(lsy_bot * iscale - lcur_sy_bot * iscale) / m_denom;
    m_bot = lm_bot * Log::pow2(8);
    cur_sy_bot = lcur_sy_bot * lh_over_2_times_256 + screen_height / 2 * 256;
    draw_after_fully_clipped(cur_sx, cur_sy_top, lm_top, m_top, cur_sy_bot,
                             lm_bot, m_bot, sx);
  }
}

static void draw_wall(uint8_t cur_px, uint16_t cur_sy_top, int16_t m_top,
                      uint16_t cur_sy_bot, int16_t m_bot, uint8_t px);

static void draw_after_fully_clipped(uint16_t cur_sx, uint16_t cur_sy_top,
                                     Log lm_top, int16_t m_top,
                                     uint16_t cur_sy_bot, Log lm_bot,
                                     int16_t m_bot, uint16_t sx) {
  DEBUG("m_top: %d:%d, m_bot: %d:%d\n", m_top >> 8, m_top & 0xff, m_bot >> 8,
        m_bot & 0xff);

  DEBUG("cur_sx: %u:%u, cur_sy_top: %u:%u, cur_sy_bot: %u:%u, sx: %u:%u\n",
        cur_sx >> 8, cur_sx & 0xff, cur_sy_top >> 8, cur_sy_top & 0xff,
        cur_sy_bot >> 8, cur_sy_bot & 0xff, sx >> 8, sx & 0xff);

  // Adjust to the next pixel center.
  int16_t offset = 128 - cur_sx % 256;
  if (offset < 0)
    offset += 256;
  if (offset) {
    cur_sx += offset;
    Log loffset = Log(offset);
    cur_sy_top += lm_top * loffset;
    cur_sy_bot += lm_bot * loffset;
  }

  uint8_t cur_px = cur_sx / 256;
  uint8_t px = sx % 256 <= 128 ? sx / 256 : sx / 256 + 1;
  draw_wall(cur_px, cur_sy_top, m_top, cur_sy_bot, m_bot, px);
}

static void draw_wall(uint8_t cur_px, uint16_t cur_sy_top, int16_t m_top,
                      uint16_t cur_sy_bot, int16_t m_bot, uint8_t px) {
  uint8_t *fb_col = &fb_next[cur_px / 2 * 30];
  for (; cur_px < px; ++cur_px, cur_sy_top += m_top, cur_sy_bot += m_bot) {
    uint8_t cur_py_top = cur_sy_top / 256;
    if (cur_sy_top % 256 > 128)
      ++cur_py_top;
    uint8_t cur_py_bot = cur_sy_bot / 256 + 1;
    if (cur_sy_bot % 256 <= 128)
      --cur_py_bot;
    if (cur_px & 1) {
      draw_column_odd(0, 3, 1, fb_col, cur_py_top, cur_py_bot);
      fb_col += 30;
    } else {
      draw_column_even(0, 3, 1, fb_col, cur_py_top, cur_py_bot);
    }
  }
}

// Note: y_bot is exclusive.
void draw_column_odd(uint8_t ceil_color, uint8_t wall_color,
                     uint8_t floor_color, uint8_t *col, uint8_t y_top,
                     uint8_t y_bot) {
  uint8_t i;
  if (!ceil_color) {
    i = y_top / 2;
  } else {
    for (i = 0; i < y_top / 2; i++) {
      col[i] &= 0b00001111;
      col[i] &= ceil_color << 6 | ceil_color << 4;
    }
  }
  if (y_top & 1) {
    col[i] &= 0b00001111;
    col[i] |= wall_color << 6 | ceil_color << 4;
    i++;
  }
  if (!wall_color) {
    i = y_bot / 2;
  } else {
    for (; i < y_bot / 2; i++) {
      col[i] &= 0b00001111;
      col[i] |= wall_color << 6 | wall_color << 4;
    }
  }
  if (y_bot & 1) {
    col[i] &= 0b00001111;
    col[i] |= floor_color << 6 | wall_color << 4;
    i++;
  }
  if (!floor_color)
    return;
  for (; i < screen_height / 2; i++) {
    col[i] &= 0b00001111;
    col[i] |= floor_color << 6 | floor_color << 4;
  }
}

// Note: y_bot is exclusive.
void draw_column_even(uint8_t ceil_color, uint8_t wall_color,
                      uint8_t floor_color, uint8_t *col, uint8_t y_top,
                      uint8_t y_bot) {
  uint8_t i;
  if (!ceil_color) {
    i = y_top / 2;
  } else {
    for (i = 0; i < y_top / 2; i++) {
      col[i] &= 0b11110000;
      col[i] &= ceil_color << 2 | ceil_color;
    }
  }
  if (y_top & 1) {
    col[i] &= 0b11110000;
    col[i] |= wall_color << 2 | ceil_color;
    i++;
  }
  if (!wall_color) {
    i = y_bot / 2;
  } else {
    for (; i < y_bot / 2; i++) {
      col[i] &= 0b11110000;
      col[i] |= wall_color << 2 | wall_color;
    }
  }
  if (y_bot & 1) {
    col[i] &= 0b11110000;
    col[i] |= floor_color << 2 | wall_color;
    i++;
  }
  if (!floor_color)
    return;
  for (; i < screen_height / 2; i++) {
    col[i] &= 0b11110000;
    col[i] |= floor_color << 2 | floor_color;
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
  // The horizontal frustum is x = +-w, and we'd like the vertical frustum to be
  // y = +-w, instead of y = +-ht/wt*w. Accordingly, scale here by wt/ht.
  *cc_y = Log(player.z - z) * lw_over_h;
}
