#include "overhead.h"

#include <bank.h>
#include <neslib.h>

#include "draw.h"
#include "log.h"
#include "screen.h"
#include "trig.h"
#include "util.h"

void overhead::begin() {
  set_prg_bank(1);
  oam_spr(128 - 4, 120 - 4, 10, 0);
}
void overhead::end() { oam_spr(0, 0xff, 0, 0); }

#pragma clang section text = ".prg_rom_1.text" rodata = ".prg_rom_1.rodata"

static uint16_t vc_width = 800;
Log lvc_width = Log(false, 19751);
static uint16_t vc_height = 750;
constexpr Log lh_over_w(false, 191);

constexpr Log scale_up_factor(false, -311);
constexpr Log scale_down_factor(false, 281);

static void setup_camera();
static void move_to(uint16_t x, uint16_t y);
static void draw_to(uint16_t x, uint16_t y);

void overhead::render(const Map &map) {
  clear_screen();
  setup_camera();
  Sector &s = *map.player_sector;
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

static Log lcamera_cos, lcamera_sin;
static void setup_camera() {
  // The player is facing up in the overhead view.
  uint16_t ang = PI_OVER_2 - player.ang;
  lcamera_cos = lcos(ang);
  lcamera_sin = lsin(ang);
}

static void w_to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y);
static bool on_screen(int16_t vc_x, int16_t vc_y);
static bool line_visible(int16_t vc_x1, int16_t vc_y1, int16_t vc_x2,
                         int16_t vc_y2);
static void clip(int16_t vc_x, int16_t vc_y, int16_t *clip_vc_x,
                 int16_t *clip_vc_y);
static void vc_to_s(int16_t vc_x, int16_t vc_y, uint16_t *nsx, uint16_t *sy);

static int16_t cur_vc_x;
static int16_t cur_vc_y;
static bool cur_on_screen;

static void move_to(uint16_t x, uint16_t y) {
  w_to_vc(x, y, &cur_vc_x, &cur_vc_y);
  if (on_screen(cur_vc_x, cur_vc_y)) {
    cur_on_screen = true;
    uint16_t sx, sy;
    vc_to_s(cur_vc_x, cur_vc_y, &sx, &sy);
    line_move_to(sx, sy);
  } else {
    cur_on_screen = false;
  }
}

static void draw_to(uint16_t x, uint16_t y) {
  int16_t vc_x, vc_y;
  w_to_vc(x, y, &vc_x, &vc_y);

  // If the current is on screen, the line is definitely visible.
  if (!cur_on_screen && !line_visible(vc_x, vc_y, cur_vc_x, cur_vc_y)) {
    cur_vc_x = vc_x;
    cur_vc_y = vc_y;
    return;
  }

  int16_t unclipped_vc_x = vc_x;
  int16_t unclipped_vc_y = vc_y;

  if (!cur_on_screen) {
    clip(vc_x, vc_y, &cur_vc_x, &cur_vc_y);
    uint16_t sx, sy;
    vc_to_s(cur_vc_x, cur_vc_y, &sx, &sy);
    line_move_to(sx, sy);
  }
  if (!on_screen(vc_x, vc_y)) {
    cur_on_screen = false;
    clip(cur_vc_x, cur_vc_y, &vc_x, &vc_y);
  } else {
    cur_on_screen = true;
  }

  cur_vc_x = unclipped_vc_x;
  cur_vc_y = unclipped_vc_y;

  uint16_t sx, sy;
  vc_to_s(vc_x, vc_y, &sx, &sy);
  line_draw_to(3, sx, sy);
}

void w_to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y) {
  Log ltx = x - (player.x >> 8);
  Log lty = y - (player.y >> 8);
  *vc_x = lcamera_cos * ltx - lcamera_sin * lty;
  *vc_y = -lcamera_sin * ltx - lcamera_cos * lty;
}

static bool on_screen(int16_t vc_x, int16_t vc_y) {
  int16_t x_bound = vc_width;
  int16_t y_bound = vc_height;
  return abs(vc_x) <= x_bound && abs(vc_y) <= y_bound;
}

static bool line_visible(int16_t vc_x1, int16_t vc_y1, int16_t vc_x2,
                         int16_t vc_y2) {
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

static void clip(int16_t vc_x, int16_t vc_y, int16_t *clip_vc_x,
                 int16_t *clip_vc_y) {
  int16_t x_bound = vc_width;
  int16_t y_bound = vc_height;

  Log lm = Log(*clip_vc_y - vc_y) / Log(*clip_vc_x - vc_x);
  if (*clip_vc_x < -x_bound) {
    *clip_vc_y += Log(-x_bound - *clip_vc_x) * lm;
    *clip_vc_x = -x_bound;
  }
  if (*clip_vc_x > x_bound) {
    *clip_vc_y -= Log(*clip_vc_x - x_bound) * lm;
    *clip_vc_x = x_bound;
  }
  if (*clip_vc_y < -y_bound) {
    *clip_vc_x += Log(-y_bound - *clip_vc_y) / lm;
    *clip_vc_y = -y_bound;
  }
  if (*clip_vc_y > y_bound) {
    *clip_vc_x -= Log(*clip_vc_y - y_bound) / lm;
    *clip_vc_y = y_bound;
  }
}

static void vc_to_s(int16_t vc_x, int16_t vc_y, uint16_t *sx, uint16_t *sy) {
  Log lsx = Log(vc_x) / lvc_width;
  Log lsy = Log(vc_y) / lvc_width;
  *sx = lsx * Log::pow2(13) + screen_width / 2 * 256 + screen_guard;
  *sy = lsy * Log::pow2(13) + screen_width / 2 * 256 + screen_guard;
}

void overhead::scale_up() {
  lvc_width *= scale_up_factor;
  vc_width = lvc_width;
  vc_height = lvc_width * lh_over_w;
}
void overhead::scale_down() {
  lvc_width *= scale_down_factor;
  vc_width = lvc_width;
  vc_height = lvc_width * lh_over_w;
}
