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

static bool cur_left_of_left;
static bool cur_right_of_right;
static bool cur_bot_above_top;
static bool cur_top_below_bot;
static bool cur_top_above_top;
static bool cur_bot_below_bot;

constexpr int16_t wall_top_z = 80;
constexpr int16_t wall_bot_z = 20;

#define DEBUG_CC(PREFIX, NAME)                                                 \
  DEBUG("%s: (%d,[%d,%d],%d)\n", PREFIX, NAME##_x, NAME##_y_top, NAME##_y_bot, \
        NAME##_w)

// 60/64
constexpr Log lh_over_w(false, -191);

static void move_to(uint16_t x, uint16_t y) {
  DEBUG("Move to: (%u,%u)\n", x, y);
  xy_to_cc(x, y, &cur_cc_x, &cur_cc_w);
  z_to_cc(wall_top_z, &cur_cc_y_top);
  z_to_cc(wall_bot_z, &cur_cc_y_bot);
  DEBUG_CC("Moved to CC:", cur_cc);

  Log lcur_cc_x = cur_cc_x;
  Log lcur_cc_y_bot = cur_cc_y_bot;
  Log lcur_cc_y_top = cur_cc_y_top;
  Log lcur_cc_w = cur_cc_w;

  cur_left_of_left = cur_cc_x < -cur_cc_w;
  cur_right_of_right = cur_cc_x > cur_cc_w;

  Log cur_sy_bot = lcur_cc_y_bot / lcur_cc_w;
  Log cur_sy_top = lcur_cc_y_top / lcur_cc_w;

  cur_bot_above_top = cur_sy_bot < -lh_over_w;
  cur_top_below_bot = cur_sy_top > lh_over_w;
  cur_top_above_top = cur_sy_top < -lh_over_w;
  cur_bot_below_bot = cur_sy_bot > lh_over_w;
}

void left_edge(int32_t *begin, int32_t *delta);
void right_edge(int16_t x, int16_t w, int32_t *begin, int32_t *delta);
void top_edge(int16_t x, int16_t y_top, int16_t w, int32_t *begin, int32_t *dx,
              int32_t *dy, int32_t *nextcol);
void bot_edge(int16_t x, int16_t y_bot, int16_t w, int32_t *begin, int32_t *dx,
              int32_t *dy, int32_t *nextcol);

template <bool x_odd>
void draw_column(uint8_t color, uint8_t *col, uint8_t y_top, uint8_t y_bot);

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

  Log lcur_cc_x = cur_cc_x;
  Log lcur_cc_w = cur_cc_w;
  Log lcc_x = cc_x;
  Log lcc_w = cc_w;

  Log lsy_top = Log(cc_y_top) / lcc_w;
  Log lsy_bot = Log(cc_y_bot) / lcc_w;

  bool left_of_left = cc_x < -cc_w;
  bool right_of_right = cc_x > cc_w;
  bool bot_above_top = lsy_bot < -lh_over_w;
  bool top_below_bot = lsy_top > lh_over_w;
  bool top_above_top = lsy_top < -lh_over_w;
  bool bot_below_bot = lsy_bot > lh_over_w;

  // There is homogeneous weirdness when both w are zero. Disallow.
  if (cc_w <= 0 && cur_cc_w <= 0)
    goto done;

  {
    if (cur_left_of_left && left_of_left ||
        cur_right_of_right && right_of_right ||
        cur_bot_above_top && bot_above_top ||
        cur_top_below_bot && top_below_bot) {
      DEBUG("Frustum cull: %d %d %d %d\n", cur_left_of_left && left_of_left,
            cur_right_of_right && right_of_right,
            cur_bot_above_top && bot_above_top,
            cur_top_below_bot && top_below_bot);
      goto done;
    }

    // Use cross product to backface cull.
    if (lcur_cc_x * lcc_w >= lcur_cc_w * lcc_x) {
      DEBUG("Backface cull.\n");
      goto done;
    }

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
        Log t_num = -cur_cc_w - cur_cc_x;
        Log t_denom = dx + dw;
        cur_cc_x += ldx * t_num / t_denom;
        lcur_cc_x = cur_cc_x;
        cur_cc_y_top += ldy_top * t_num / t_denom;
        cur_cc_y_bot += ldy_bot * t_num / t_denom;
        cur_cc_w = -cur_cc_x;
        lcur_cc_w = cur_cc_w;
        DEBUG_CC("Clipped Cur", cur_cc);
      }
      if (right_of_right) {
        DEBUG("Wall crosses right frustum edge. Clipping.\n");
        DEBUG_CC("Cur", cur_cc);
        DEBUG_CC("Next", cc);
        Log t_num = cc_w - cc_x;
        Log t_denom = dx - dw;
        cc_x += ldx * t_num / t_denom;
        lcc_x = cc_x;
        cc_y_top += ldy_top * t_num / t_denom;
        cc_y_bot += ldy_bot * t_num / t_denom;
        cc_w = cc_x;
        lcc_w = cc_w;
        lsy_top = Log(cc_y_top) / lcc_w;
        lsy_bot = Log(cc_y_bot) / lcc_w;
        DEBUG_CC("Clipped Next", cc);
      }
    }

    Log lcur_sx = lcur_cc_x / lcur_cc_w;
    Log lsx = lcc_x / lcc_w;
    Log lcur_sy_top = Log(cur_cc_y_top) / lcur_cc_w;
    Log lcur_sy_bot = Log(cur_cc_y_bot) / lcur_cc_w;

    uint16_t sx = lcur_sx * Log::pow2(13) + screen_width / 2 * 256;
    uint16_t sx_right = lsx * Log::pow2(13) + screen_width / 2 * 256;

    Log lm_top;
    int16_t m_top;
    Log lm_bot;
    int16_t m_bot;
    uint16_t sy_top;
    uint16_t sy_bot;
    Log iscale = Log::pow2(13);
    Log m_denom = lsx * iscale - lcur_sx * iscale;
    if (cur_top_above_top && top_above_top) {
      lm_top = Log::zero();
      m_top = 0;
      sy_top = 0;
      cur_top_above_top = top_above_top = false;
      DEBUG("Clipped top.\n");
    } else {
      lm_top = Log(lsy_top * iscale - lcur_sy_top * iscale) / m_denom;
      m_top = lm_top * Log::pow2(8);
      sy_top = lcur_sy_top * Log::pow2(13) + screen_height / 2 * 256;
    }
    if (cur_bot_below_bot && bot_below_bot) {
      lm_bot = Log::zero();
      m_bot = 0;
      sy_bot = screen_height * 256;
      cur_bot_below_bot = bot_below_bot = false;
      DEBUG("Clipped bot.\n");
    } else {
      lm_bot = Log(lsy_bot * iscale - lcur_sy_bot * iscale) / m_denom;
      m_bot = lm_bot * Log::pow2(8);
      sy_bot = lcur_sy_bot * Log::pow2(13) + screen_height / 2 * 256;
    }

    DEBUG("m_top: %d:%d, m_bot: %d:%d\n", m_top >> 8, m_top & 0xff, m_bot >> 8,
          m_bot & 0xff);

#if 0
    uint8_t sy_top[64];
    uint8_t sy_bot[64];
    int16_t sx = (int32_t)cur_cc_x * screen_width * 256 / cur_cc_w;
    int16_t sy_top, sy_bot;
    bool cur_top_on_screen = !cur_top_above_top && !cur_top_below_bot;
    bool cur_bot_on_screen = !cur_bot_above_top && !cur_bot_below_bot;
    if (cur_top_on_screen || cur_bot_on_screen) {
      if (cur_top_on_screen)
        sy_top = (int32_t)cur_cc_y_top * screen_width * 256 / cur_cc_w;
      if (cur_bot_on_screen)
        sy_bot = (int32_t)cur_cc_y_bot * screen_width * 256 / cur_cc_w;
    }
#endif

    DEBUG("sx: %u:%u, sy_top: %u:%u, sy_bot: %u:%u, sx_right: %u:%u\n", sx >> 8,
          sx & 0xff, sy_top >> 8, sy_top & 0xff, sy_bot >> 8, sy_bot & 0xff,
          sx_right >> 8, sy_bot & 0xff);

    // Top is inclusive, bot is exclusive.
    uint8_t pix_y_tops[64];
    uint8_t pix_y_bots[64];

    // Adjust to the next pixel center.
    int16_t offset = 128 - sx % 256;
    if (offset < 0)
      offset += 256;
    if (offset) {
      sx += offset;
      Log loffset = Log(offset);
      sy_top += lm_top * loffset;
      sy_bot += lm_bot * loffset;
    }

    uint8_t pix_x_begin = sx / 256;
    uint8_t pix_x_end =
        sx_right % 256 <= 128 ? sx_right / 256 : sx_right / 256 + 1;
    for (; sx < sx_right; sx += 256, sy_top += m_top, sy_bot += m_bot) {
      uint8_t top_pix = sy_top / 256;
      if (sy_top % 256 > 128)
        ++top_pix;
      uint8_t bot_pix = sy_bot / 256 + 1;
      if (sy_bot % 256 <= 128)
        --bot_pix;
      pix_y_tops[sx / 256] = top_pix;
      pix_y_bots[sx / 256] = bot_pix;
    }

    uint8_t *fb_col = &fb_next[pix_x_begin / 2 * 30];
    for (uint8_t pix_x = pix_x_begin; pix_x < pix_x_end; ++pix_x) {
      if (pix_x & 1) {
        draw_column<true>(0, fb_col, 0, pix_y_tops[pix_x]);
        draw_column<true>(1, fb_col, pix_y_tops[pix_x], pix_y_bots[pix_x]);
        draw_column<true>(0, fb_col, pix_y_bots[pix_x], screen_height);
        fb_col += 30;
      } else {
        draw_column<false>(0, fb_col, 0, pix_y_tops[pix_x]);
        draw_column<false>(1, fb_col, pix_y_tops[pix_x], pix_y_bots[pix_x]);
        draw_column<false>(0, fb_col, pix_y_bots[pix_x], screen_height);
      }
    }
  }

done:
  cur_cc_x = orig_cc_x;
  cur_cc_y_top = orig_cc_y_top;
  cur_cc_y_bot = orig_cc_y_bot;
  cur_cc_w = orig_cc_w;
  cur_left_of_left = left_of_left;
  cur_right_of_right = right_of_right;
  cur_bot_above_top = bot_above_top;
  cur_top_below_bot = top_below_bot;
  cur_top_above_top = top_above_top;
  cur_bot_below_bot = bot_below_bot;
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

static void xy_to_cc(uint16_t x, uint16_t y, int16_t *cc_x, int16_t *cc_w) {
  int16_t tx = x - player.x;
  int16_t ty = y - player.y;
  int16_t vx = mul_cos(-player.ang, tx) - mul_sin(-player.ang, ty);
  int16_t vy = mul_sin(-player.ang, tx) + mul_cos(-player.ang, ty);
  *cc_x = -vy;
  *cc_w = vx;
}

static void z_to_cc(uint16_t z, int16_t *cc_y) { *cc_y = player.z - z; }

// The following is an interpretation of Olano and Greer's homogenous
// rasterization for wall quads.
//
// The wall is a rectangle in 3d (x,y,w) clip space. Each edge of the rectangle
// is a line in clip space, but since the space is a homogenous representation
// of the project space, they each define a plane through the origin. A normal
// for that plane is a vector orthogonal to the vertices of the edge, i.e.,
// their cross product. The plane consists of all points orthogonal to the
// cross product, i.e., the dot product is zero.
//
// Accordingly, if the edge runs from (x0,y0,w0) to (x1,y1,w1), then the
// equation for the plane is: [x,y,z] * [x0,y0,w0] x [x1,y1,w1] = 0. Expanding,
// this gives: (1,2,3), (y0w1 - w0y1) * x + (w0x1 - x0w1) * y + (x0y1 - y0x1) *
// w = 0
//
// Note that the left hand side of the equation is linear in clip space, and it
// crosses from negative to positive exactly where it intersects the edge plane.
// (The equation can be negated if necessary to make this so.) We can then
// compute this function for all four edges, and a clip point is in the wall's
// frustum iff all four LHS are positive.
//
// Dividing the LHS by w gives us an expression that is linear in screen space,
// not clip space:
// (y0w1 - w0y1) * x / w + (w0x1 - x0w1) * y / w + (x0y1 - y0x1) This can be
// linearly interpolated for each pixel on the screen. Note that if w < 0, the
// sign would be flipped from the true value of the expression in clip space.
// But, we only care to query pixel positions within the view frustum, so we can
// always select some arbitrary positive w, and this ensures that the sign of
// this expression agrees with its clip space equivalent.
//
// I call it "clip space", but by this I just mean that the homogenous
// coordinate has taken on the z necessary to perform the perspective divide. In
// this approach, it's not necessary to perform this divide (or actually, any
// divisions at all!), nor is it necessary to normalize the clip space so that
// the frustum bouds are +-1.
//
// Let's arbitrarily take w to be 1; the expression then becomes:
// (y0w1 - w0y1) * x + (w0x1 - x0w1) * y + (x0y1 - y0x1)
// We can give these coefficients names:
// a = y0w1 - w0y1
// b = w0x1 - x0w1
// c = x0y1 - y0x1
//
// If w=1, then the screen frustum bounds are {[-1,1],[-ht/wt, ht/wt]}.  This is
// divided evenly into w columns and h rows, each 2/wt in width and 2/wt in
// height (let's not do rectangular pixels for now).
//
// Accordingly, the top left pixel has center [-1+2/wt/2, -ht/wt+2/wt/2] =
// [-1+1/wt, -ht/wt+1/wt]. To advance screen x or y by one pixel, we add 2/wt.
//
// Plugging in the top left pixel gives:
// a * (-1+1/wt) + b * (-ht/wt+1/wt) + c
//
// Now, another observation: we still only care about the sign of this
// expression, and multiplying the start and the increments by a positive
// constant does not change the sign. Accordingly, we can multply by wt, and
// get:
//
// Top left: a * (1-wt) + b * (1-ht) + c * wt
// = a - a*wt + b*(1-ht) + c * wt
// = a + (c - a)*wt + b*(1-ht)
// dX = 2a, dY = 2b

int32_t edge_begin(int32_t a, int32_t b, int32_t c) {
  return a + (c - a) * (int32_t)screen_width + b * (1 - (int32_t)screen_height);
}

void left_edge(int32_t *begin, int32_t *delta) {
  int32_t a = cur_cc_w;
  int32_t c = -cur_cc_x;
  *begin = edge_begin(a, 0, c);
  *delta = 2 * a;
}

void right_edge(int16_t x, int16_t w, int32_t *begin, int32_t *delta) {
  int32_t a = -w;
  int32_t c = x;
  *begin = edge_begin(a, 0, c);
  *delta = 2 * a;
}

void top_edge(int16_t x, int16_t y_top, int16_t w, int32_t *begin, int32_t *dx,
              int32_t *dy, int32_t *nextcol) {
  int32_t a = (int32_t)cur_cc_y_top * w - (int32_t)cur_cc_w * y_top;
  int32_t b = (int32_t)cur_cc_w * x - (int32_t)cur_cc_x * w;
  int32_t c = (int32_t)cur_cc_x * y_top - (int32_t)cur_cc_y_top * x;
  *begin = edge_begin(a, b, c);
  *dx = 2 * a;
  *dy = 2 * b;
  *nextcol = *dx - screen_height * *dy;
}

void bot_edge(int16_t x, int16_t y_bot, int16_t w, int32_t *begin, int32_t *dx,
              int32_t *dy, int32_t *nextcol) {
  int32_t a = (int32_t)cur_cc_w * y_bot - (int32_t)cur_cc_y_bot * w;
  int32_t b = (int32_t)cur_cc_x * w - (int32_t)cur_cc_w * x;
  int32_t c = (int32_t)cur_cc_y_bot * x - (int32_t)cur_cc_x * y_bot;
  *begin = edge_begin(a, b, c);
  *dx = 2 * a;
  *dy = 2 * b;
  *nextcol = *dx - screen_height * *dy;
}
