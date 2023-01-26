#include "perspective.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "draw.h"
#include "log.h"
#include "screen.h"
#include "trig.h"
#include "util.h"

#pragma clang section text = ".prg_rom_0.text" rodata = ".prg_rom_0.rodata"

// #define DEBUG_FILE
#include "debug.h"

#define DEBUG_CC(PREFIX, NAME)                                                 \
  DEBUG("%s: (%d,[%d,%d],%d)\n", PREFIX, NAME##_x, NAME##_y_top, NAME##_y_bot, \
        NAME##_w)

static void clear_col_z();
static void setup_camera();
static void move_to(uint16_t x, uint16_t y);
static void draw_to(uint16_t x, uint16_t y);

static int16_t ceiling_z;
static int16_t floor_z;

__attribute__((noinline)) void perspective::render(const Map &map) {
  DEBUG("Begin frame.\n");
  clear_screen();
  clear_col_z();

  setup_camera();
  Sector &s = *map.player_sector;
  ceiling_z = s.ceiling_z;
  floor_z = s.floor_z;
  Wall *begin_loop = nullptr;
  for (uint16_t j = 0; j < s.num_walls; j++) {
    Wall &w = s.walls[j];
    if (w.begin_loop) {
      if (begin_loop)
        draw_to(begin_loop->x, begin_loop->y);
      move_to(w.x, w.y);
      begin_loop = &w;
    } else {
      draw_to(w.x, w.y);
    }
  }
  if (begin_loop)
    draw_to(begin_loop->x, begin_loop->y);
}

static void xy_to_cc(uint16_t x, uint16_t y, int16_t *cc_x, int16_t *cc_w);
static void z_to_cc(uint16_t z, int16_t *cc_y);

static int16_t cur_cc_x;
static int16_t cur_cc_y_top;
static int16_t cur_cc_y_bot;
static int16_t cur_cc_w;

static uint8_t col_z_lo[screen_width];
static uint8_t col_z_hi[screen_width];

static void clear_col_z() {
  memset(col_z_lo, 0xff, sizeof col_z_lo);
  memset(col_z_hi, 0xff, sizeof col_z_hi);
}

static Log lcamera_cos, lcamera_sin;
static void setup_camera() {
  lcamera_cos = lcos(-player.ang);
  lcamera_sin = lsin(-player.ang);
}

// 64/60
constexpr Log lw_over_h(false, 191);

// 60/2*256
constexpr Log lh_over_2_times_256(false, 26433);

static void move_to(uint16_t x, uint16_t y) {
  DEBUG("Move to: (%u,%u)\n", x, y);
  xy_to_cc(x, y, &cur_cc_x, &cur_cc_w);
  z_to_cc(ceiling_z, &cur_cc_y_top);
  z_to_cc(floor_z, &cur_cc_y_bot);
  DEBUG_CC("Moved to CC:", cur_cc);
}

template <bool is_odd>
void draw_column(uint8_t ceil_color, uint8_t wall_color, uint8_t floor_color,
                 uint8_t *col, uint8_t y_top, uint8_t y_bot, bool on_bg);

static void draw_wall(uint8_t cur_px, uint16_t sz, int16_t zm, uint8_t px);

static void clip_bot_and_draw_to(Log lm_top, Log lm_bot, int16_t cc_x,
                                 int16_t cc_y_top, int16_t cc_y_bot,
                                 int16_t cc_w);

static void clip_and_rasterize_edge(uint8_t *edge, int16_t cur_cc_x,
                                    int16_t cur_cc_y, int16_t cur_cc_w,
                                    int16_t cc_x, int16_t cc_y, int16_t cc_w);
static void rasterize_edge(uint8_t *edge, int16_t cur_cc_x, int16_t cur_cc_y,
                           int16_t cur_cc_w, int16_t cc_x, int16_t cc_y,
                           int16_t cc_w);

static uint16_t lsx_to_sx(Log lsx);
static uint16_t lsy_to_sy(Log lsy);
static uint16_t lsz_to_sz(Log lsy);

static uint8_t s_to_p(uint16_t s);

static uint8_t py_tops[64];
static uint8_t py_bots[64];

__attribute__((noinline)) static void draw_to(uint16_t x, uint16_t y) {
  DEBUG("Draw to: (%u,%u)\n", x, y);

  int16_t cc_x, cc_w;
  xy_to_cc(x, y, &cc_x, &cc_w);

  int16_t cc_y_top, cc_y_bot;
  z_to_cc(ceiling_z, &cc_y_top);
  z_to_cc(floor_z, &cc_y_bot);
  DEBUG_CC("Draw to", cc);

  int16_t orig_cc_x = cc_x;
  int16_t orig_cc_y_top = cc_y_top;
  int16_t orig_cc_y_bot = cc_y_bot;
  int16_t orig_cc_w = cc_w;

  bool cur_left_of_left = cur_cc_x < -cur_cc_w;
  bool cur_right_of_right = cur_cc_x > cur_cc_w;
  bool left_of_left = cc_x < -cc_w;
  bool right_of_right = cc_x > cc_w;

  // There is homogeneous weirdness when both w are zero. Disallow.
  if (cc_w <= 0 && cur_cc_w <= 0)
    goto done;

  {
    if (cur_left_of_left && left_of_left ||
        cur_right_of_right && right_of_right) {
      DEBUG("LR frustum cull round 1: %d %d\n",
            cur_left_of_left && left_of_left,
            cur_right_of_right && right_of_right);
      goto done;
    }

    // Negative w can cause naive frustum culling not to work due to a litany of
    // corner cases, so clip to w=1 (no division by zero please) and cull again.
    if (cur_cc_w < 1 || cc_w < 1) {
      int16_t dx = cc_x - cur_cc_x;
      int16_t dy_top = cc_y_top - cur_cc_y_top;
      int16_t dy_bot = cc_y_bot - cur_cc_y_bot;
      int16_t dw = cc_w - cur_cc_w;
      Log ldx = dx;
      Log ldy_top = dy_top;
      Log ldy_bot = dy_bot;
      Log ldw = dw;

      if (cur_cc_w < 1) {
        // w = 1 = cur_w + dw*t
        Log t = Log(1 - cur_cc_w) / ldw;
        cur_cc_x += ldx * t;
        cur_cc_y_top += ldy_top * t;
        cur_cc_y_bot += ldy_bot * t;
        cur_cc_w = 1;
        cur_left_of_left = cur_cc_x < -cur_cc_w;
        cur_right_of_right = cur_cc_x > cur_cc_w;
        DEBUG("Clipped cur to w=1.\n");
        DEBUG_CC("Cur", cur_cc);
      } else {
        Log t = Log(1 - cc_w) / ldw;
        cc_x += ldx * t;
        cc_y_top += ldy_top * t;
        cc_y_bot += ldy_bot * t;
        cc_w = 1;
        left_of_left = cc_x < -cc_w;
        right_of_right = cc_x > cc_w;
        DEBUG("Clipped next to w=1.\n");
        DEBUG_CC("Next", cc);
      }

      if (cur_left_of_left && left_of_left ||
          cur_right_of_right && right_of_right) {
        DEBUG("LR frustum cull round 2: %d %d\n",
              cur_left_of_left && left_of_left,
              cur_right_of_right && right_of_right);
        goto done;
      }
    }

    if (Log(cur_cc_x) / Log(cur_cc_w) >= Log(cc_x) / Log(cc_w)) {
      DEBUG("Backface cull.\n");
      goto done;
    }

    // Note that the logarithmic sx values range from [-1, 1], while the
    // unsigned values range from [0, screen_width*256].

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
        // r(t)_x = -r(t)_w
        // cur_x + dxt = -cur_w - dw*t;
        // t*(dx + dw) = -cur_w - cur_x
        // t = (-cur_w - cur_x) / (dx + dw)
        // Since the wall cannot be parallel to the left frustum edge, dw + dx
        // != 0.
        Log t = Log(-cur_cc_w - cur_cc_x) / Log(dx + dw);
        cur_cc_x += ldx * t;
        cur_cc_y_top += ldy_top * t;
        cur_cc_y_bot += ldy_bot * t;
        cur_cc_w = -cur_cc_x;
        DEBUG_CC("Clipped Cur", cur_cc);
      }
      if (right_of_right) {
        DEBUG("Wall crosses right frustum edge. Clipping.\n");
        DEBUG_CC("Cur", cur_cc);
        DEBUG_CC("Next", cc);
        Log t = Log(cc_w - cc_x) / Log(dx - dw);
        cc_x += ldx * t;
        cc_y_top += ldy_top * t;
        cc_y_bot += ldy_bot * t;
        cc_w = cc_x;
        DEBUG_CC("Clipped Next", cc);
      }
    }

    clip_and_rasterize_edge(py_tops, cur_cc_x, cur_cc_y_top, cur_cc_w, cc_x,
                            cc_y_top, cc_w);
    clip_and_rasterize_edge(py_bots, cur_cc_x, cur_cc_y_bot, cur_cc_w, cc_x,
                            cc_y_bot, cc_w);

    Log lcur_sx = Log(cur_cc_x) / Log(cur_cc_w);
    Log lsx = Log(cc_x) / Log(cc_w);
    uint16_t cur_sx = lsx_to_sx(lcur_sx);
    uint16_t sx = lsx_to_sx(lsx);

    Log lcur_sz = -Log::one() / Log(cur_cc_w);
    Log lsz = -Log::one() / Log(cc_w);

    uint16_t cur_sz = lsz_to_sz(lcur_sz);
    uint16_t sz = lsz_to_sz(lsz);

    DEBUG("cur_sz: %u, sz: %u\n", lcur_sz, lsz);
    int16_t zm = Log(sz - cur_sz) / Log(sx - cur_sx) * Log::pow2(8);
    DEBUG("zm: %d\n", zm);

    draw_wall(s_to_p(cur_sx), cur_sz, zm, s_to_p(sx));
  }

done:
  cur_cc_x = orig_cc_x;
  cur_cc_y_top = orig_cc_y_top;
  cur_cc_y_bot = orig_cc_y_bot;
  cur_cc_w = orig_cc_w;
}

static uint16_t lsx_to_sx(Log lsx) {
  if (lsx.abs() == Log::one())
    return lsx.sign ? 0 : screen_width * 256;
  else
    return lsx * Log::pow2(13) + screen_width / 2 * 256;
}

static uint16_t lsy_to_sy(Log lsy) {
  if (lsy.abs() == Log::one())
    return lsy.sign ? 0 : screen_height * 256;
  else
    return lsy * lh_over_2_times_256 + screen_height / 2 * 256;
}

static uint16_t lsz_to_sz(Log lsz) { return (32768 + lsz * Log::pow2(15)) * 2; }

static uint8_t s_to_p(uint16_t s) {
  return s % 256 <= 128 ? s / 256 : s / 256 + 1;
}

static void clip_and_rasterize_edge(uint8_t *edge, int16_t cur_cc_x,
                                    int16_t cur_cc_y, int16_t cur_cc_w,
                                    int16_t cc_x, int16_t cc_y, int16_t cc_w) {
  bool cur_above_top = cur_cc_y < -cur_cc_w;
  bool cur_below_bot = cur_cc_y > cur_cc_w;
  bool above_top = cc_y < -cc_w;
  bool below_bot = cc_y > cc_w;

  if (!cur_above_top && !cur_below_bot && !above_top && !below_bot) {
    rasterize_edge(edge, cur_cc_x, cur_cc_y, cur_cc_w, cc_x, cc_y, cc_w);
  } else if (cur_above_top && above_top) {
    DEBUG("Clipped both sides to top.\n");
    clip_and_rasterize_edge(edge, cur_cc_x, -cur_cc_w, cur_cc_w, cc_x, -cc_w,
                            cc_w);
  } else if (cur_below_bot && below_bot) {
    DEBUG("Clipped both sides to bot.\n");
    clip_and_rasterize_edge(edge, cur_cc_x, cur_cc_w, cur_cc_w, cc_x, cc_w,
                            cc_w);
  } else {
    int16_t dx = cc_x - cur_cc_x;
    int16_t dy = cc_y - cur_cc_y;
    int16_t dw = cc_w - cur_cc_w;

    int16_t isect_cc_x;
    int16_t isect_cc_y;
    int16_t isect_cc_w;
    if (cur_above_top || above_top) {
      DEBUG("Clipped to top.\n");
      // r(t) = cur + vt
      // r(t)_y = -r(t)_w
      // cur_y + dyt = -cur_w - dw*t;
      // t*(dy + dw) = -cur_w - cur_y
      // t = (-cur_w - cur_y) / (dy + dw)
      Log t = Log(-cur_cc_w - cur_cc_y) / Log(dy + dw);
      isect_cc_x = cur_cc_x + Log(dx) * t;
      isect_cc_y = cur_cc_y + Log(dy) * t;
      isect_cc_w = -isect_cc_y;
    } else {
      DEBUG("Clipped to bot.\n");
      // r(t) = cur + vt
      // r(t)_y = r(t)_w
      // cur_y + dyt = cur_w + dw*t;
      // t*(dy - dw) = cur_w - cur_y
      // t = (cur_w - cur_y) / (dy - dw)
      Log t = Log(cur_cc_w - cur_cc_y) / Log(dy - dw);
      isect_cc_x = cur_cc_x + Log(dx) * t;
      isect_cc_y = cur_cc_y + Log(dy) * t;
      isect_cc_w = isect_cc_y;
    }
    DEBUG("isect: (%d,%d,%d)\n", isect_cc_x, isect_cc_y, isect_cc_w);
    if (cur_above_top || cur_below_bot) {
      DEBUG("Clipped left side of edge.\n");
      clip_and_rasterize_edge(edge, cur_cc_x,
                              cur_above_top ? -cur_cc_w : cur_cc_w, cur_cc_w,
                              isect_cc_x, isect_cc_y, isect_cc_w);
      clip_and_rasterize_edge(edge, isect_cc_x, isect_cc_y, isect_cc_w, cc_x,
                              cc_y, cc_w);
    } else {
      DEBUG("Clipped right side of edge.\n");
      clip_and_rasterize_edge(edge, cur_cc_x, cur_cc_y, cur_cc_w, isect_cc_x,
                              isect_cc_y, isect_cc_w);
      clip_and_rasterize_edge(edge, isect_cc_x, isect_cc_y, isect_cc_w, cc_x,
                              above_top ? -cc_w : cc_w, cc_w);
    }
  }
}

__attribute__((noinline)) static void
rasterize_edge(uint8_t *edge, int16_t cur_cc_x, int16_t cur_cc_y,
               int16_t cur_cc_w, int16_t cc_x, int16_t cc_y, int16_t cc_w) {
  Log lcur_cc_w = cur_cc_w;
  Log lcc_w = cc_w;

  Log lcur_sx = Log(cur_cc_x) / lcur_cc_w;
  Log lcur_sy = Log(cur_cc_y) / lcur_cc_w;

  Log lsx = Log(cc_x) / lcc_w;
  Log lsy = Log(cc_y) / lcc_w;

  uint16_t cur_sx = lsx_to_sx(lcur_sx);
  uint16_t cur_sy = lsy_to_sy(lcur_sy);
  uint16_t sx = lsx_to_sx(lsx);
  uint16_t sy = lsy_to_sy(lsy);
  DEBUG("Rasterize edge: (%u,%u) to (%u,%u)\n", cur_sx, cur_sy, sx, sy);

  Log lm = Log(sy - cur_sy) / Log(sx - cur_sx);
  int16_t m = lm * Log::pow2(8);
  DEBUG("m: %d\n", m);

  // Adjust to the next pixel center.
  int16_t offset = 128 - cur_sx % 256;
  if (offset < 0)
    offset += 256;
  DEBUG("Offset: %d\n", offset);
  if (offset) {
    cur_sx += offset;
    Log loffset = Log(offset);
    if ((m >= 0 || cur_sy >= -m) &&
        (m <= 0 || cur_sy <= screen_height * 256 - m))
      cur_sy += lm * loffset;
    DEBUG("New cur: %u,%u\n", cur_sx, cur_sy);
  }

  bool off_screen = false;
  uint8_t px = s_to_p(sx);
  for (uint8_t cur_px = s_to_p(cur_sx); cur_px < px; ++cur_px) {
    uint8_t cur_py = cur_sy / 256;
    if (cur_sy % 256 > 128)
      ++cur_py;
    edge[cur_px] = cur_py;
    if (!off_screen) {
      if (m < 0 && cur_sy < -m) {
        off_screen = true;
        cur_sy = 0;
      } else if (m > 0 && cur_sy > (int16_t)(screen_height * 256) - m) {
        off_screen = true;
        cur_sy = screen_height * 256;
      } else {
        cur_sy += m;
      }
    }
  }
}

static void draw_wall(uint8_t cur_px, uint16_t sz, int16_t zm, uint8_t px) {
  DEBUG("x: [%d,%d), sz: %u, zm: %d\n", cur_px, px, sz, zm);
  uint8_t *fb_col = &fb_next[cur_px / 2 * 30];
  for (; cur_px < px; ++cur_px, sz += zm) {
    uint16_t col_z = col_z_hi[cur_px] << 8 | col_z_lo[cur_px];
    bool on_bg = col_z == 0xffff;
    if (sz >= col_z) {
      if (cur_px & 1)
        fb_col += 30;
      continue;
    }
    col_z_lo[cur_px] = sz & 0xff;
    col_z_hi[cur_px] = sz >> 8;
    if (cur_px & 1) {
      draw_column<true>(0, 3, 1, fb_col, py_tops[cur_px], py_bots[cur_px],
                        on_bg);
      fb_col += 30;
    } else {
      draw_column<false>(0, 3, 1, fb_col, py_tops[cur_px], py_bots[cur_px],
                         on_bg);
    }
  }
}

// Note: y_bot is exclusive.
template <bool is_odd>
void draw_column(uint8_t ceil_color, uint8_t wall_color, uint8_t floor_color,
                 uint8_t *col, uint8_t y_top, uint8_t y_bot, bool on_bg) {
  uint8_t i;
  if (!ceil_color && on_bg) {
    i = y_top / 2;
  } else {
    for (i = 0; i < y_top / 2; i++) {
      if (is_odd) {
        col[i] &= 0b00001111;
        col[i] &= ceil_color << 6 | ceil_color << 4;
      } else {
        col[i] &= 0b11110000;
        col[i] &= ceil_color << 2 | ceil_color;
      }
    }
  }
  if (i == screen_height / 2)
    return;
  if (y_bot != y_top && y_top & 1) {
    if (is_odd) {
      col[i] &= 0b00001111;
      col[i] |= wall_color << 6 | ceil_color << 4;
    } else {
      col[i] &= 0b11110000;
      col[i] |= wall_color << 2 | ceil_color;
    }
    i++;
  }
  if (!wall_color && on_bg) {
    i = y_bot / 2;
  } else {
    for (; i < y_bot / 2; i++) {
      if (is_odd) {
        col[i] &= 0b00001111;
        col[i] |= wall_color << 6 | wall_color << 4;
      } else {
        col[i] &= 0b11110000;
        col[i] |= wall_color << 2 | wall_color;
      }
    }
  }
  if (i == screen_height / 2)
    return;
  if (y_bot != y_top && y_bot & 1) {
    if (is_odd) {
      col[i] &= 0b00001111;
      col[i] |= floor_color << 6 | wall_color << 4;
    } else {
      col[i] &= 0b11110000;
      col[i] |= floor_color << 2 | wall_color;
    }
    i++;
  }
  if (!floor_color && on_bg)
    return;
  for (; i < screen_height / 2; i++) {
    if (is_odd) {
      col[i] &= 0b00001111;
      col[i] |= floor_color << 6 | floor_color << 4;
    } else {
      col[i] &= 0b11110000;
      col[i] |= floor_color << 2 | floor_color;
    }
  }
}

static void xy_to_cc(uint16_t x, uint16_t y, int16_t *cc_x, int16_t *cc_w) {
  Log ltx = x - (player.x >> 8);
  Log lty = y - (player.y >> 8);
  int16_t vx = lcamera_cos * ltx - lcamera_sin * lty;
  int16_t vy = lcamera_sin * ltx + lcamera_cos * lty;
  *cc_x = -vy;
  *cc_w = vx;
}

static void z_to_cc(uint16_t z, int16_t *cc_y) {
  // The horizontal frustum is x = +-w, and we'd like the vertical frustum to
  // be y = +-w, instead of y = +-ht/wt*w. Accordingly, scale here by wt/ht.
  *cc_y = Log((player.z >> 8) - z) * lw_over_h;
}
