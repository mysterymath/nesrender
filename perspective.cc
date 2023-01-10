#include "perspective.h"

#include <stdint.h>
#include <stdio.h>

#include "draw.h"
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

constexpr int16_t wall_top_z = 80;
constexpr int16_t wall_bot_z = 20;

#define DEBUG_CC(PREFIX, NAME)                                                 \
  DEBUG("%s: (%d,[%d,%d],%d)\n", PREFIX, NAME##_x, NAME##_y_top, NAME##_y_bot, \
        NAME##_w)

static void move_to(uint16_t x, uint16_t y) {
  DEBUG("Move to: (%u,%u)\n", x, y);
  xy_to_cc(x, y, &cur_cc_x, &cur_cc_w);
  z_to_cc(wall_top_z, &cur_cc_y_top);
  z_to_cc(wall_bot_z, &cur_cc_y_bot);
  DEBUG_CC("Moved to CC:", cur_cc);
}

void left_edge(int32_t *begin, int32_t *delta);
void right_edge(int16_t x, int16_t w, int32_t *begin, int32_t *delta);
void top_edge(int16_t x, int16_t y_top, int16_t w, int32_t *begin, int32_t *dx,
              int32_t *dy, int32_t *nextcol);
void bot_edge(int16_t x, int16_t y_bot, int16_t w, int32_t *begin, int32_t *dx,
              int32_t *dy, int32_t *nextcol);

static void draw_to(uint16_t x, uint16_t y) {
  DEBUG("Draw to: (%u,%u)\n", x, y);

  int16_t cc_x, cc_w;
  xy_to_cc(x, y, &cc_x, &cc_w);
  int16_t cc_y_top, cc_y_bot;
  z_to_cc(wall_top_z, &cc_y_top);
  z_to_cc(wall_bot_z, &cc_y_bot);
  DEBUG_CC("Draw to", cc);

  int32_t left, ldx;
  left_edge(&left, &ldx);
  int32_t right, rdx;
  right_edge(cc_x, cc_w, &right, &rdx);
  int32_t top, tdx, tdy, tnc;
  top_edge(cc_x, cc_y_top, cc_w, &top, &tdx, &tdy, &tnc);
  int32_t bot, bdx, bdy, bnc;
  bot_edge(cc_x, cc_y_bot, cc_w, &bot, &bdx, &bdy, &bnc);
  DEBUG("ldx: %ld, rdx: %ld\n", ldx, rdx);
  DEBUG("tdx: %ld, tdy: %ld, tnc: %ld\n", tdx, tdy, tnc);
  DEBUG("bdx: %ld, bdy: %ld, bnc: %ld\n", bdx, bdy, bnc);
  uint8_t *fb_col = fb_next;
  for (uint8_t sx = 0; sx < screen_width; left += ldx, right += rdx,
               fb_col = (sx & 1) ? fb_col + 30 : fb_col, ++sx) {
    DEBUG("sx: %u, left: %ld, right: %ld, top: %ld, bot: %ld\n", sx, left,
          right, top, bot);
    if (left < 0) {
      top += tdx;
      bot += bdx;
      continue;
    }
    if (right <= 0)
      break;
    for (uint8_t sy = 0; sy < screen_height; sy++, top += tdy, bot += bdy) {
      if (sx & 1 && sy & 1) {
        fb_col[sy / 2] &= 0b00111111;
        if (top >= 0 && bot > 0)
          fb_col[sy / 2] |= 0b01000000;
      } else if (sx & 1) {
        fb_col[sy / 2] &= 0b11001111;
        if (top >= 0 && bot > 0)
          fb_col[sy / 2] |= 0b00010000;
      } else if (sy & 1) {
        fb_col[sy / 2] &= 0b11110011;
        if (top >= 0 && bot > 0)
          fb_col[sy / 2] |= 0b00000100;
      } else {
        fb_col[sy / 2] &= 0b11111100;
        if (top >= 0 && bot > 0)
          fb_col[sy / 2] |= 0b00000001;
      }
    }
    top += tnc;
    bot += bnc;
  }

  cur_cc_x = cc_x;
  cur_cc_y_top = cc_y_top;
  cur_cc_y_bot = cc_y_bot;
  cur_cc_w = cc_w;
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
  if (a < 0) {
    a = -a;
    c = -c;
  }
  *begin = edge_begin(a, 0, c);
  *delta = 2 * a;
}

void right_edge(int16_t x, int16_t w, int32_t *begin, int32_t *delta) {
  int32_t a = -w;
  int32_t c = x;
  if (a > 0) {
    a = -a;
    c = -c;
  }
  *begin = edge_begin(a, 0, c);
  *delta = 2 * a;
}

void top_edge(int16_t x, int16_t y_top, int16_t w, int32_t *begin, int32_t *dx,
              int32_t *dy, int32_t *nextcol) {
  int32_t a = (int32_t)cur_cc_y_top * w - (int32_t)cur_cc_w * y_top;
  int32_t b = (int32_t)cur_cc_w * x - (int32_t)cur_cc_x * w;
  int32_t c = (int32_t)cur_cc_x * y_top - (int32_t)cur_cc_y_top * x;
  if (b < 0) {
    a = -a;
    b = -b;
    c = -c;
  }
  *begin = edge_begin(a, b, c);
  *dx = 2 * a;
  *dy = 2 * b;
  *nextcol = *dx - screen_height * *dy;
}

void bot_edge(int16_t x, int16_t y_bot, int16_t w, int32_t *begin, int32_t *dx,
              int32_t *dy, int32_t *nextcol) {
  int32_t a = (int32_t)cur_cc_y_bot * w - (int32_t)cur_cc_w * y_bot;
  int32_t b = (int32_t)cur_cc_w * x - (int32_t)cur_cc_x * w;
  int32_t c = (int32_t)cur_cc_x * y_bot - (int32_t)cur_cc_y_bot * x;
  if (b > 0) {
    a = -a;
    b = -b;
    c = -c;
  }
  *begin = edge_begin(a, b, c);
  *dx = 2 * a;
  *dy = 2 * b;
  *nextcol = *dx - screen_height * *dy;
}
