#include "perspective.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "draw.h"
#include "log.h"
#include "map.h"
#include "screen.h"
#include "trig.h"
#include "util.h"

#pragma clang section text = ".prg_rom_0.text" rodata = ".prg_rom_0.rodata"

// #define DEBUG_FILE
#include "debug.h"

#define DEBUG_CC(PREFIX, NAME)                                                 \
  DEBUG("%s: (%d,[%d,%d],%d)\n", PREFIX, NAME##_x, NAME##_y_top, NAME##_y_bot, \
        NAME##_w)

extern "C" void clear_col_z();
static void setup_camera();
static void draw_sector();
static void begin_loop();
static void draw_wall();
static void next_sector();
extern "C" void clear_coverage();
static void update_coverage();

static const Sector *sector;
bool sector_is_portal;

static const Wall *wall;
static const Wall *next_wall;

static const Sector *portals[64];

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

__attribute__((noinline)) void perspective::render(const Map &map) {
  DEBUG("Begin frame.\n");
  clear_col_z();
  clear_coverage();

  setup_camera();

  sector = map.player_sector;
  sector_is_portal = false;
  draw_sector();

  next_sector();
  while (sector) {
    clear_col_z();
    update_coverage();
    draw_sector();
    next_sector();
  }
}

static void next_sector() {
  sector = nullptr;
  uint8_t i;
  for (i = 0; i < ARRAY_LENGTH(portals); i++) {
    if (portals[i]) {
      sector = portals[i];
      break;
    }
  }
  if (!sector)
    return;
  sector_is_portal = true;
}

static uint8_t py_tops[64];
static uint8_t py_ceiling_steps[64];
static uint8_t py_floor_steps[64];
static uint8_t py_bots[64];
static bool has_ceiling_step, has_floor_step;

uint8_t coverage_py_tops[64];
uint8_t coverage_py_bots[64];

static void update_coverage() {
  for (uint8_t i = 0; i < ARRAY_LENGTH(coverage_py_tops); i++) {
    if (!portals[i])
      continue;
    coverage_py_tops[i] = has_ceiling_step ? py_ceiling_steps[i] : py_tops[i];
    coverage_py_bots[i] = has_floor_step ? py_floor_steps[i] : py_bots[i];
  }
}

static void draw_sector() {
  const Wall *loop_begin = nullptr;
  for (uint16_t j = 0; j < sector->num_walls; j++) {
    wall = &sector->walls[j];
    if (wall->begin_loop) {
      loop_begin = wall;
      begin_loop();
    }
    next_wall = (j + 1 == sector->num_walls || sector->walls[j + 1].begin_loop)
                    ? loop_begin
                    : &sector->walls[j + 1];
    draw_wall();
  }
}

static void xy_to_cc(uint16_t x, uint16_t y, int16_t *cc_x, int16_t *cc_w);
static void z_to_cc(uint16_t z, int16_t *cc_y);

static int16_t cur_cc_x;
static int16_t cur_cc_y_top;
static int16_t cur_cc_y_bot;
static int16_t cur_cc_w;

uint8_t col_z_lo[screen_width];
uint8_t col_z_hi[screen_width];

static Log lcamera_cos, lcamera_sin;
static void setup_camera() {
  lcamera_cos = lcos(-player.ang);
  lcamera_sin = lsin(-player.ang);
}

// 64/60
constexpr Log lw_over_h(false, 191);

// 60/2*256
constexpr Log lh_over_2_times_256(false, 26433);

static void begin_loop() {
  DEBUG("Begin loop: (%u,%u)\n", wall->x, wall->y);
  xy_to_cc(wall->x, wall->y, &cur_cc_x, &cur_cc_w);
  z_to_cc(sector->ceiling_z, &cur_cc_y_top);
  z_to_cc(sector->floor_z, &cur_cc_y_bot);
  DEBUG_CC("Moved to CC:", cur_cc);
}

template <bool is_odd>
void draw_column(uint8_t *col, uint8_t wall_color, uint8_t cur_px);

static void screen_draw_wall(uint8_t cur_px, uint16_t sz, int16_t zm,
                             uint8_t px);

static void clip_bot_and_draw_to(Log lm_top, Log lm_bot, int16_t cc_x,
                                 int16_t cc_y_top, int16_t cc_y_bot,
                                 int16_t cc_w);

static void clip_and_rasterize_edge(uint8_t *edge, int16_t cur_cc_x,
                                    int16_t cur_cc_y, int16_t cur_cc_w,
                                    uint16_t cur_sx, int16_t cc_x, int16_t cc_y,
                                    int16_t cc_w, uint16_t sx);
static void rasterize_edge(uint8_t *edge, uint16_t cur_sx, uint16_t cur_sy,
                           uint16_t sx, uint16_t sy);

static uint16_t lsx_to_sx(Log lsx);
static uint16_t lsy_to_sy(Log lsy);
static uint16_t lsz_to_sz(Log lsy);

static uint8_t s_to_p(uint16_t s);

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

__attribute__((noinline)) static void draw_wall() {
  DEBUG("Draw to: (%u,%u)\n", next_wall->x, next_wall->y);

  int16_t cc_x, cc_w;
  xy_to_cc(next_wall->x, next_wall->y, &cc_x, &cc_w);

  int16_t cc_y_top, cc_y_bot;
  z_to_cc(sector->ceiling_z, &cc_y_top);
  z_to_cc(sector->floor_z, &cc_y_bot);
  DEBUG_CC("Draw to", cc);

  int16_t orig_cc_x = cc_x;
  int16_t orig_cc_y_top = cc_y_top;
  int16_t orig_cc_y_bot = cc_y_bot;
  int16_t orig_cc_w = cc_w;

  const uint8_t LEFT = 1 << 0;
  const uint8_t RIGHT = 1 << 1;
  const uint8_t BEHIND = 1 << 2;

  uint8_t cur_outcode = 0;
  if (cur_cc_x < -cur_cc_w)
    cur_outcode |= LEFT;
  else if (cur_cc_x > cur_cc_w)
    cur_outcode |= RIGHT;
  if (cur_cc_w < 1)
    cur_outcode |= BEHIND;

  uint8_t outcode = 0;
  if (cc_x < -cc_w)
    outcode |= LEFT;
  else if (cc_x > cc_w)
    outcode |= RIGHT;
  if (cc_w <= 1)
    outcode |= BEHIND;

  // Frustum cull.
  if (cur_outcode & outcode) {
    DEBUG("Round 1 frustum cull: %d %d\n", cur_outcode, outcode);
    goto done;
  }

  {
    // Negative w can cause naive frustum culling not to work due to a litany of
    // corner cases, so clip to w=1 (no division by zero please) and cull again.
    if ((cur_outcode | outcode) & BEHIND) {
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
        cur_outcode = 0;
        if (cur_cc_x < -cur_cc_w)
          cur_outcode |= LEFT;
        if (cur_cc_x > cur_cc_w)
          cur_outcode |= RIGHT;
        DEBUG("Clipped cur to w=1.\n");
        DEBUG_CC("Cur", cur_cc);
      } else {
        Log t = Log(1 - cc_w) / ldw;
        cc_x += ldx * t;
        cc_y_top += ldy_top * t;
        cc_y_bot += ldy_bot * t;
        cc_w = 1;
        outcode = 0;
        if (cc_x < -cc_w)
          outcode |= LEFT;
        if (cc_x > cc_w)
          outcode |= RIGHT;
        DEBUG("Clipped next to w=1.\n");
        DEBUG_CC("Next", cc);
      }

      if (cur_outcode & outcode) {
        DEBUG("Round 2 frustum cull: %d %d\n", cur_outcode, outcode);
        goto done;
      }
    }

    if (Log(cur_cc_x) / Log(cur_cc_w) >= Log(cc_x) / Log(cc_w)) {
      DEBUG("Backface cull.\n");
      goto done;
    }

    // Note that the logarithmic sx values range from [-1, 1], while the
    // unsigned values range from [0, screen_width*256].

    if (cur_outcode & LEFT | outcode & RIGHT) {
      int16_t dx = cc_x - cur_cc_x;
      int16_t dy_top = cc_y_top - cur_cc_y_top;
      int16_t dy_bot = cc_y_bot - cur_cc_y_bot;
      int16_t dw = cc_w - cur_cc_w;
      Log ldx = dx;
      Log ldy_top = dy_top;
      Log ldy_bot = dy_bot;
      Log ldw = dw;

      if (cur_outcode & LEFT) {
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
      if (outcode & RIGHT) {
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

    uint16_t cur_sx = lsx_to_sx(Log(cur_cc_x) / Log(cur_cc_w));
    uint16_t sx = lsx_to_sx(Log(cc_x) / Log(cc_w));

    clip_and_rasterize_edge(py_tops, cur_cc_x, cur_cc_y_top, cur_cc_w, cur_sx,
                            cc_x, cc_y_top, cc_w, sx);
    clip_and_rasterize_edge(py_bots, cur_cc_x, cur_cc_y_bot, cur_cc_w, cur_sx,
                            cc_x, cc_y_bot, cc_w, sx);

    if (wall->portal) {
      has_ceiling_step = wall->portal->ceiling_z < sector->ceiling_z;
      if (has_ceiling_step) {
        // TODO: Slopes.
        int16_t ceiling_cc_y;
        z_to_cc(wall->portal->ceiling_z, &ceiling_cc_y);
        clip_and_rasterize_edge(py_ceiling_steps, cur_cc_x, ceiling_cc_y,
                                cur_cc_w, cc_x, cur_sx, ceiling_cc_y, cc_w, sx);
      }
      has_floor_step = wall->portal->floor_z > sector->floor_z;
      if (has_floor_step) {
        // TODO: Slopes.
        int16_t floor_cc_y;
        z_to_cc(wall->portal->floor_z, &floor_cc_y);
        clip_and_rasterize_edge(py_floor_steps, cur_cc_x, floor_cc_y, cur_cc_w,
                                cur_sx, cc_x, floor_cc_y, cc_w, sx);
      }
    }

    Log lcur_sz = -Log::one() / Log(cur_cc_w);
    Log lsz = -Log::one() / Log(cc_w);

    uint16_t cur_sz = lsz_to_sz(lcur_sz);
    uint16_t sz = lsz_to_sz(lsz);

    int16_t zm = Log(sz - cur_sz) / Log(sx - cur_sx) * Log::pow2(8);

    screen_draw_wall(s_to_p(cur_sx), cur_sz, zm, s_to_p(sx));
  }

done:
  cur_cc_x = orig_cc_x;
  cur_cc_y_top = orig_cc_y_top;
  cur_cc_y_bot = orig_cc_y_bot;
  cur_cc_w = orig_cc_w;
}

static uint16_t lsz_to_sz(Log lsz) { return (32768 + lsz * Log::pow2(15)) * 2; }

static uint8_t s_to_p(uint16_t s) {
  return s % 256 <= 128 ? s / 256 : s / 256 + 1;
}

static void clip_and_rasterize_edge(uint8_t *edge, int16_t cur_cc_x,
                                    int16_t cur_cc_y, int16_t cur_cc_w,
                                    uint16_t cur_sx, int16_t cc_x, int16_t cc_y,
                                    int16_t cc_w, uint16_t sx) {
  const uint8_t CUR_ABOVE_TOP = 1 << 0;
  const uint8_t CUR_BELOW_BOT = 1 << 1;
  const uint8_t ABOVE_TOP = 1 << 2;
  const uint8_t BELOW_BOT = 1 << 3;
  uint8_t outcode = 0;
  if (cur_cc_y < -cur_cc_w)
    outcode |= CUR_ABOVE_TOP;
  else if (cur_cc_y > cur_cc_w)
    outcode |= CUR_BELOW_BOT;
  if (cc_y < -cc_w)
    outcode |= ABOVE_TOP;
  else if (cc_y > cc_w)
    outcode |= BELOW_BOT;

  if ((outcode & (CUR_ABOVE_TOP | ABOVE_TOP)) == (CUR_ABOVE_TOP | ABOVE_TOP)) {
    DEBUG("Clipped both sides to top.\n");
    cur_cc_y = -cur_cc_w;
    cc_y = -cc_w;
    outcode &= ~(CUR_ABOVE_TOP | ABOVE_TOP);
  } else if ((outcode & (CUR_BELOW_BOT | BELOW_BOT)) ==
             (CUR_BELOW_BOT | BELOW_BOT)) {
    DEBUG("Clipped both sides to bot.\n");
    cur_cc_y = cur_cc_w;
    cc_y = cc_w;
    outcode &= ~(CUR_BELOW_BOT | BELOW_BOT);
  }

  uint16_t cur_sy = lsy_to_sy(Log(cur_cc_y) / Log(cur_cc_w));
  uint16_t sy = lsy_to_sy(Log(cc_y) / Log(cc_w));

  if (!outcode) {
    rasterize_edge(edge, cur_sx, cur_sy, sx, sy);
    return;
  }

  int16_t dx = cc_x - cur_cc_x;
  int16_t dy = cc_y - cur_cc_y;
  int16_t dw = cc_w - cur_cc_w;

  int16_t top_isect_cc_x;
  int16_t top_isect_cc_y;
  int16_t top_isect_cc_w;
  uint16_t top_isect_sx;
  bool isect_top = outcode & (CUR_ABOVE_TOP | ABOVE_TOP);
  if (isect_top) {
    DEBUG("Clipped to top.\n");
    // r(t) = cur + vt
    // r(t)_y = -r(t)_w
    // cur_y + dyt = -cur_w - dw*t;
    // t*(dy + dw) = -cur_w - cur_y
    // t = (-cur_w - cur_y) / (dy + dw)
    Log t = Log(-cur_cc_w - cur_cc_y) / Log(dy + dw);
    top_isect_cc_x = cur_cc_x + Log(dx) * t;
    top_isect_cc_y = cur_cc_y + Log(dy) * t;
    top_isect_cc_w = -top_isect_cc_y;
    top_isect_sx = lsx_to_sx(Log(top_isect_cc_x) / Log(top_isect_cc_w));
    DEBUG("top_isect: (%d,%d,%d)\n", top_isect_cc_x, top_isect_cc_y,
          top_isect_cc_w);
    if (outcode & CUR_ABOVE_TOP) {
      cur_cc_y = -cur_cc_w;
      cur_sy = 0;
    } else {
      cc_y = -cc_w;
      sy = 0;
    }
  }

  int16_t bot_isect_cc_x;
  int16_t bot_isect_cc_y;
  int16_t bot_isect_cc_w;
  uint16_t bot_isect_sx;
  bool isect_bot = outcode & (CUR_BELOW_BOT | BELOW_BOT);
  if (isect_bot) {
    DEBUG("Clipped to bot.\n");
    // r(t) = cur + vt
    // r(t)_y = r(t)_w
    // cur_y + dyt = cur_w + dw*t;
    // t*(dy - dw) = cur_w - cur_y
    // t = (cur_w - cur_y) / (dy - dw)
    Log t = Log(cur_cc_w - cur_cc_y) / Log(dy - dw);
    bot_isect_cc_x = cur_cc_x + Log(dx) * t;
    bot_isect_cc_y = cur_cc_y + Log(dy) * t;
    bot_isect_cc_w = bot_isect_cc_y;
    bot_isect_sx = lsx_to_sx(Log(bot_isect_cc_x) / Log(bot_isect_cc_w));
    DEBUG("bot_isect: (%d,%d,%d)\n", bot_isect_cc_x, bot_isect_cc_y,
          bot_isect_cc_w);
    if (outcode & CUR_BELOW_BOT) {
      cur_cc_y = cur_cc_w;
      cur_sy = screen_height * 256;
    } else {
      cc_y = cc_w;
      sy = screen_height * 256;
    }
  }

  if (isect_top && isect_bot) {
    if (outcode & CUR_ABOVE_TOP) {
      rasterize_edge(edge, cur_sx, 0, top_isect_sx, 0);
      rasterize_edge(edge, top_isect_sx, 0, bot_isect_sx, screen_height * 256);
      rasterize_edge(edge, bot_isect_sx, screen_height * 256, sx,
                     screen_height * 256);
    } else {
      rasterize_edge(edge, cur_sx, screen_height * 256, bot_isect_sx,
                     screen_height * 256);
      rasterize_edge(edge, bot_isect_sx, screen_height * 256, top_isect_sx, 0);
      rasterize_edge(edge, top_isect_sx, 0, sx, sy);
    }
  } else if (isect_top) {
    rasterize_edge(edge, cur_sx, cur_sy, top_isect_sx, 0);
    rasterize_edge(edge, top_isect_sx, 0, sx, sy);
  } else {
    rasterize_edge(edge, cur_sx, cur_sy, bot_isect_sx, screen_height * 256);
    rasterize_edge(edge, bot_isect_sx, screen_height * 256, sx, sy);
  }
}

static void rasterize_edge(uint8_t *edge, uint16_t cur_sx, uint16_t cur_sy,
                           uint16_t sx, uint16_t sy) {
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

  int16_t icur_sy = cur_sy;
  uint8_t px = s_to_p(sx);
  uint8_t cur_px;
  bool off_top;
  for (cur_px = s_to_p(cur_sx); cur_px < px; ++cur_px) {
    if (icur_sy < 0) {
      off_top = true;
      break;
    }
    if (icur_sy > screen_height * 256) {
      off_top = false;
      break;
    }
    uint8_t cur_py = icur_sy >> 8;
    if ((cur_sy & 0xff) > 128)
      ++cur_py;
    if (cur_py < coverage_py_tops[cur_px])
      cur_py = coverage_py_tops[cur_px];
    if (cur_py > coverage_py_bots[cur_px])
      cur_py = coverage_py_bots[cur_px];
    edge[cur_px] = cur_py;
    icur_sy += m;
  }
  if (cur_px != px) {
    uint8_t cur_py = off_top ? 0 : screen_height;
    if (cur_py < coverage_py_tops[cur_px])
      cur_py = coverage_py_tops[cur_px];
    if (cur_py > coverage_py_bots[cur_px])
      cur_py = coverage_py_bots[cur_px];
    for (; cur_px < px; ++cur_px)
      edge[cur_px] = cur_py;
  }
}

static void screen_draw_wall(uint8_t cur_px, uint16_t sz, int16_t zm,
                             uint8_t px) {
  DEBUG("x: [%d,%d), sz: %u, zm: %d\n", cur_px, px, sz, zm);
  uint8_t *fb_col = &fb_next[cur_px / 2 * 30];
  bool is_left_edge = cur_px;
  for (; cur_px < px; fb_col = cur_px & 1 ? fb_col + 30 : fb_col, ++cur_px,
                      sz += zm, is_left_edge = false) {
    if (sector_is_portal && portals[cur_px] != sector)
      continue;
    uint16_t col_z = col_z_hi[cur_px] << 8 | col_z_lo[cur_px];
    if (sz >= col_z)
      continue;
    col_z_lo[cur_px] = sz & 0xff;
    col_z_hi[cur_px] = sz >> 8;

    portals[cur_px] = wall->portal;

    if (cur_px & 1)
      draw_column<true>(fb_col, is_left_edge ? 0 : wall->color, cur_px);
    else
      draw_column<false>(fb_col, is_left_edge ? 0 : wall->color, cur_px);
  }
}

template <bool is_odd>
void draw_column(uint8_t *col, uint8_t wall_color, uint8_t cur_px) {
  auto draw_solid_column =
      is_odd ? draw_solid_column_odd : draw_solid_column_even;

  uint8_t y_top = py_tops[cur_px];
  uint8_t y_bot = py_bots[cur_px];

  if (wall->portal) {
    draw_solid_column(col, sector->ceiling_color, coverage_py_tops[cur_px],
                      y_top);
    if (has_ceiling_step)
      draw_solid_column(col, wall_color, y_top, py_ceiling_steps[cur_px]);
    if (has_floor_step)
      draw_solid_column(col, wall_color, py_floor_steps[cur_px], y_bot);
    draw_solid_column(col, sector->floor_color, y_bot,
                      coverage_py_bots[cur_px]);
    return;
  }

  draw_solid_column(col, sector->ceiling_color, coverage_py_tops[cur_px],
                    y_top);
  draw_solid_column(col, wall_color, y_top, y_bot);
  draw_solid_column(col, sector->floor_color, y_bot, coverage_py_bots[cur_px]);
}

#if 0
template <bool is_odd>
void draw_solid_column(uint8_t *col, uint8_t color, uint8_t y_top,
                       uint8_t y_bot) {
  if (y_bot == y_top)
    return;
  uint8_t i = y_top / 2;
  if (y_top & 1) {
    if (is_odd)
      col[i] &= 0b00111111;
    else
      col[i] &= 0b11110011;
    i++;
  } else {
    if (is_odd) {
      col[i] &= 0b00001111;
      col[i] |= color << 6;
    } else {
      col[i] &= 0b11110000;
      col[i] |= color << 2;
    }
    i++;
  }
  for (; i < y_bot / 2; i++) {
    if (is_odd) {
      col[i] &= 0b00001111;
      col[i] |= color << 6 | color << 4;
    } else {
      col[i] &= 0b11110000;
      col[i] |= color << 2 | color;
    }
  }
  if (y_bot & 1) {
    if (is_odd) {
      col[i] &= 0b11001111;
      col[i] |= color << 4;
    } else {
      col[i] &= 0b11111100;
      col[i] |= color;
    }
    i++;
  }
}
#endif

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
