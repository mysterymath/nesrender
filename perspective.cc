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
}

static void draw_to(uint16_t x, uint16_t y) {
  DEBUG("Draw to: (%u,%u)\n", x, y);

  int16_t cc_x, cc_w;
  xy_to_cc(x, y, &cc_x, &cc_w);
  int16_t cc_y_top, cc_y_bot;
  z_to_cc(wall_top_z, &cc_y_top);
  z_to_cc(wall_bot_z, &cc_y_bot);
  DEBUG_CC("Draw to", cc);

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

static void z_to_cc(uint16_t z, int16_t *cc_y) { *cc_y = z - player.z; }

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
// Accordingly, if the edge runs from (x0,y0,w0) to (x1,y1,w1), then the equation for the plane is:
// [x,y,z] * [x0,y0,w0] x [x1,y1,w1] = 0.
// Expanding, this gives:
// (1,2,3),
// (y0w1 - w0y1) * x + (w0x1 - x0w1) * y + (x0y1 - y0x1) * w = 0
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
// 
// If w=1, then the screen frustum bounds are {[-1,1],[-ht/wt, ht/wt]}.  This is
// divided evenly into w columns and h rows, each 2/wt in width and 2/wt in
// height (let's not do rectangular pixels for now). 
//
// Accordingly, the top left pixel has bounds {[-1,-1+2/wt],[ht/wt-2/wt, ht/wt]}.
// It's center is [-1+2/wt/2, ht/wt-2/wt/2] = [-1+1/wt, ht/wt-1/wt]
// To advance screen x or screen y by one pixel, we add 2/wt. To decrease screen
// y by one pixel, we subtract 2/wt. With thipls, we can zig zag across the
// screen, linearly interpolating along the way.
//
// We can plug this directly into the above formula.
// The value of the expression at the top left pixel center is this:
// (y0w1 - w0y1) * (-1+1/wt) + (w0x1 - x0w1) * (ht/wt-1/wt) + (x0y1 - y0x1)
//
// Incremeting X by one pixel adds:                        (y0w1 - w0y1) * 2 / wt
// Incremeting/decrementing Y by one pixel adds/subtracts: (w0x1 - x0w1) * 2 / wt
//
// Now, another observation: we still only care about the sign of this
// expression, and multiplying the start and the increments by a positive
// constant does not change the sign. Accordingly, we can multply by wt, and get:
//
// Top left: (y0w1 - w0y1) * (1-wt) + (w0x1 - x0w1) * (ht-1) + (x0y1 - y0x1) * wt
// Incremeting X by one pixel adds:                        (y0w1 - w0y1) * 2
// Incremeting/decrementing Y by one pixel adds/subtracts: (w0x1 - x0w1) * 2
//
// We can give the coefficients names:
// a = y0w1 - w0y1
// b = w0x1 - x0w1
// c = x0y1 - y0x1
// Top left: a*(1-wt) + b*(ht-1) + c*wt
// dX: a*2
// dY: b*2
//
// Finally, we can compute the coefficients for each quad edge:
// Left: (x_min,y_top,w_x_min) to (x_min, y_bot, w_x_min)
// a = y_top*w_x_min - w_x_min*y_bot = w_x_min * (y_top - y_bot)
// b = w_x_min*x_min - w_x_min * x_min = 0
// c = x_min*y_bot - y_top*x_min = x_min * (y_bot - y_top)
// Left = w_x_min * (y_top - y_bot) * x - x_min * (y_top - y_bot)
// We can divide this by (y_top - y_bot) without changing the sign:
// Left = w_x_min * x - x_min
// a = w_x_min, b = 0, c = -x_min
// Then, it's just a matter of checking whether a point along the right end is
// negative, and multiplying the coefficients by -1 if so.
// This gives:
// Top left: w_x_min*(1-wt) - x_min*wt
// dX: w_x_min*2
// dY: 0

// WIP: Let's actually try left in practice and see if it works correctly before
// doing the others.