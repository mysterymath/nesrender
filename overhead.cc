#include "overhead.h"

#include "draw.h"
#include "map.h"
#include "screen.h"
#include "util.h"

uint16_t vc_width = 100;
uint16_t vc_width_recip = (uint32_t)65536 / vc_width;
uint16_t vc_height = vc_width * screen_height / screen_width;

// Percent
constexpr uint16_t scale_speed = 10;

static void move_to(uint16_t x, uint16_t y);
static void draw_to(uint16_t x, uint16_t y);

void overhead_render() {
  clear_screen();
  move_to(100, 100);
  draw_to(100, 200);
  draw_to(200, 200);
  draw_to(200, 100);
  draw_to(100, 100);
}

static bool on_screen(int16_t vc_x, int16_t vc_y);
static bool line_visible(int16_t vc_x1, int16_t vc_y1, int16_t vc_x2, int16_t vc_y2);
static void clip(int16_t vc_x, int16_t vc_y, int16_t *clip_vc_x, int16_t *clip_vc_y);
static void to_screen(int16_t vc_x, int16_t vc_y, uint16_t *sx, uint16_t *sy);

static int16_t cur_vc_x;
static int16_t cur_vc_y;

static void move_to(uint16_t x, uint16_t y) {
  to_vc(x, y, &cur_vc_x, &cur_vc_y);
  if (on_screen(cur_vc_x, cur_vc_y)) {
    uint16_t sx, sy;
    to_screen(cur_vc_x, cur_vc_y, &sx, &sy);
    line_move_to(sx, sy);
  }
}

static void draw_to(uint16_t x, uint16_t y) {
  int16_t vc_x, vc_y;
  to_vc(x, y, &vc_x, &vc_y);

  if (!line_visible(vc_x, vc_y, cur_vc_x, cur_vc_y)) {
    cur_vc_x = vc_x;
    cur_vc_y = vc_y;
    return;
  }

  int16_t unclipped_vc_x = vc_x;
  int16_t unclipped_vc_y = vc_y;

  if (!on_screen(cur_vc_x, cur_vc_y)) {
    clip(vc_x, vc_y, &cur_vc_x, &cur_vc_y);
    uint16_t sx, sy;
    to_screen(cur_vc_x, cur_vc_y, &sx, &sy);
    line_move_to(sx, sy);
  }
  if (!on_screen(vc_x, vc_y))
    clip(cur_vc_x, cur_vc_y, &vc_x, &vc_y);

  cur_vc_x = unclipped_vc_x;
  cur_vc_y = unclipped_vc_y;

  uint16_t sx, sy;
  to_screen(vc_x, vc_y, &sx, &sy);
  line_draw_to(3, sx, sy);
}

static bool on_screen(int16_t vc_x, int16_t vc_y) {
  int16_t x_bound = vc_width;
  int16_t y_bound = vc_height;
  return abs(vc_x) <= x_bound && abs(vc_y) <= y_bound;
}

static bool line_visible(int16_t vc_x1, int16_t vc_y1, int16_t vc_x2, int16_t vc_y2) {
  int16_t x_bound = vc_width;
  int16_t y_bound = vc_height;

  if (vc_x1 < -x_bound && vc_x2 < -x_bound)
    return false;
  if (vc_x1 > x_bound && vc_x2 > x_bound)
    return false;
  if (vc_y1 < -y_bound && vc_y2 < -y_bound)
    return false;
  if (vc_y1 > y_bound && vc_y2 > y_bound)
    return false;
  return true;
}

static void clip(int16_t vc_x, int16_t vc_y, int16_t *clip_vc_x, int16_t *clip_vc_y) {
  int16_t x_bound = vc_width;
  int16_t y_bound = vc_height;

  int32_t dy = *clip_vc_y - vc_y;
  int32_t dx = *clip_vc_x - vc_x;
  if (*clip_vc_x < -x_bound) {
    *clip_vc_y += (-x_bound - *clip_vc_x) * dy / dx;
    *clip_vc_x = -x_bound;
  }
  if (*clip_vc_x > x_bound) {
    *clip_vc_y -= (*clip_vc_x - x_bound) * dy / dx;
    *clip_vc_x = x_bound;
  }
  if (*clip_vc_y < -y_bound) {
    *clip_vc_x += (-y_bound - *clip_vc_y) * dx / dy;
    *clip_vc_y = -y_bound;
  }
  if (*clip_vc_y > y_bound) {
    *clip_vc_x -= (*clip_vc_y - y_bound) * dx / dy;
    *clip_vc_y = y_bound;
  }
}

static void to_screen(int16_t vc_x, int16_t vc_y, uint16_t *sx,
                        uint16_t *sy) {
  *sx = ((int32_t)vc_x * 256 * screen_width / 2 * vc_width_recip >> 16) +
        screen_width / 2 * 256 + screen_guard;
  *sy = screen_height / 2 * 256 -
        ((int32_t)vc_y * 256 * screen_width / 2 * vc_width_recip >> 16) +
        screen_guard;
}

void overhead_scale_up() {
  vc_width = (uint32_t)vc_width * (100 - scale_speed) / 100;
  vc_width_recip = (uint32_t)65536 / vc_width;
  vc_height = vc_width * screen_height / screen_width;
}
void overhead_scale_down() {
  vc_width = (uint32_t)vc_width * (100 + scale_speed) / 100;
  vc_width_recip = (uint32_t)65536 / vc_width;
  vc_height = vc_width * screen_height / screen_width;
}
