#include <6502.h>
#include <bank.h>
#include <nes.h>
#include <nesdoug.h>
#include <neslib.h>
#include <peekpoke.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mul.h"
#include "trig.h"

// Configure for SNROM MMC1 board.
asm(".globl __chr_rom_size\n"
    "__chr_rom_size = 8\n"
    ".globl __prg_ram_size\n"
    "__prg_ram_size = 8\n");

constexpr uint16_t width = 64;
constexpr uint16_t height = 60;

uint16_t wc_width = 100;
uint16_t wc_width_recip = (uint32_t)65536 / wc_width;
uint16_t wc_height = wc_width * height / width;

// Percent
constexpr uint16_t scale_speed = 10;

uint16_t player_x = 150;
uint16_t player_y = 150;
uint16_t player_z = 50;
uint16_t player_ang = 0;

constexpr uint16_t speed = 5;
constexpr uint16_t ang_speed = 1024;

bool still_presenting;
bool overhead_view;

void render_overhead();
void render_perspective();
void init_present();
void present();

int main() {
  static const uint8_t bg_pal[16] = {0x00, 0x11, 0x16, 0x1a};
  static const uint8_t spr_pal[16] = {0x00, 0x00, 0x10, 0x30};
  ppu_off();
  set_mmc1_ctrl(0b01100);
  pal_bright(4);
  pal_bg(bg_pal);
  pal_spr(spr_pal);
  bank_spr(1);
  ppu_on_all();

  oam_spr(128 - 4, 120 - 4, 0, 0);
  init_present();

  while (true) {
    uint8_t pad_t = pad_trigger(0);
    uint8_t pad = pad_state(0);
    if (pad_t & PAD_START)
      overhead_view = !overhead_view;
    if (overhead_view && pad & PAD_B) {
      if (pad & PAD_UP) {
        wc_width = (uint32_t)wc_width * (100 - scale_speed) / 100;
        wc_width_recip = (uint32_t)65536 / wc_width;
        wc_height = wc_width * height / width;
      } else if (pad & PAD_DOWN) {
        wc_width = (uint32_t)wc_width * (100 + scale_speed) / 100;
        wc_width_recip = (uint32_t)65536 / wc_width;
        wc_height = wc_width * height / width;
      }
    } else {
      if (pad & PAD_UP) {
        player_x += mul_cos(player_ang, speed);
        player_y += mul_sin(player_ang, speed);
      } else if (pad & PAD_DOWN) {
        player_x += mul_cos(player_ang + PI, speed);
        player_y += mul_sin(player_ang + PI, speed);
      }
      if (pad & PAD_A) {
        if (pad & PAD_LEFT) {
          player_x += mul_cos(player_ang + PI_OVER_2, speed);
          player_y += mul_sin(player_ang + PI_OVER_2, speed);
        } else if (pad & PAD_RIGHT) {
          player_x += mul_cos(player_ang - PI_OVER_2, speed);
          player_y += mul_sin(player_ang - PI_OVER_2, speed);
        }
      } else {
        if (pad & PAD_LEFT)
          player_ang += ang_speed;
        else if (pad & PAD_RIGHT)
          player_ang -= ang_speed;
      }
    }

    if (!still_presenting) {
      if (overhead_view)
        render_overhead();
      else
        render_perspective();
    }
    present();
  }
}

// Values are 8:8 fixed point screen pixels.
void line_move_to(uint16_t x, uint16_t y);
void line_draw_to(uint8_t color, uint16_t x, uint16_t y);

void wall_move_to(uint8_t x, uint8_t y_top, uint8_t y_bot);
void wall_draw_to(uint8_t color, uint8_t x, uint8_t y_top, uint8_t y_bot);

void overhead_wall_move_to(uint16_t x, uint16_t y);
void overhead_wall_draw_to(uint16_t x, uint16_t y);

void perspective_wall_move_to(uint16_t x, uint16_t y);
void perspective_wall_draw_to(uint16_t x, uint16_t y);

#pragma clang section bss = ".prg_ram_0"
uint8_t fb_cur[960];
uint8_t fb_next[960];
#pragma clang section bss = ""

__attribute__((no_builtin("memset"))) void clear_screen() {
  for (int i = 0; i < 256; i++) {
    fb_next[i] = 0;
    (fb_next + 256)[i] = 0;
    (fb_next + 512)[i] = 0;
  }
  for (uint8_t i = 0; i < 192; i++)
    (fb_next + 768)[i] = 0;
}

void render_overhead() {
  clear_screen();
  overhead_wall_move_to(100, 100);
  overhead_wall_draw_to(100, 200);
  overhead_wall_draw_to(200, 200);
  overhead_wall_draw_to(200, 100);
  overhead_wall_draw_to(100, 100);
}

void render_perspective() {
  clear_screen();
  perspective_wall_move_to(100, 100);
  perspective_wall_draw_to(100, 200);
  perspective_wall_draw_to(200, 200);
  perspective_wall_draw_to(200, 100);
  perspective_wall_draw_to(100, 100);
}

void to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y);
bool on_screen(int16_t vc_x, int16_t vc_y);
bool line_on_screen(int16_t vc_x1, int16_t vc_y1, int16_t vc_x2, int16_t vc_y2);
void clip(int16_t vc_x, int16_t vc_y, int16_t *clip_vc_x, int16_t *clip_vc_y);
void overhead_to_screen(int16_t vc_x, int16_t vc_y, uint16_t *sx, uint16_t *sy);

int16_t overhead_cur_vc_x;
int16_t overhead_cur_vc_y;

template <typename T> T abs(T t) { return t < 0 ? -t : t; }

void overhead_wall_move_to(uint16_t x, uint16_t y) {
  to_vc(x, y, &overhead_cur_vc_x, &overhead_cur_vc_y);
  if (on_screen(overhead_cur_vc_x, overhead_cur_vc_y)) {
    uint16_t sx, sy;
    overhead_to_screen(overhead_cur_vc_x, overhead_cur_vc_y, &sx, &sy);
    line_move_to(sx, sy);
  }
}

void overhead_wall_draw_to(uint16_t x, uint16_t y) {
  int16_t vc_x, vc_y;
  to_vc(x, y, &vc_x, &vc_y);

  if (!line_on_screen(vc_x, vc_y, overhead_cur_vc_x, overhead_cur_vc_y)) {
    overhead_cur_vc_x = vc_x;
    overhead_cur_vc_y = vc_y;
    return;
  }

  int16_t unclipped_vc_x = vc_x;
  int16_t unclipped_vc_y = vc_y;

  if (!on_screen(overhead_cur_vc_x, overhead_cur_vc_y)) {
    clip(vc_x, vc_y, &overhead_cur_vc_x, &overhead_cur_vc_y);
    uint16_t sx, sy;
    overhead_to_screen(overhead_cur_vc_x, overhead_cur_vc_y, &sx, &sy);
    line_move_to(sx, sy);
  }
  if (!on_screen(vc_x, vc_y))
    clip(overhead_cur_vc_x, overhead_cur_vc_y, &vc_x, &vc_y);

  overhead_cur_vc_x = unclipped_vc_x;
  overhead_cur_vc_y = unclipped_vc_y;

  uint16_t sx, sy;
  overhead_to_screen(vc_x, vc_y, &sx, &sy);
  line_draw_to(3, sx, sy);
}

int16_t perspective_cur_vc_x;
int16_t perspective_cur_vc_y;

bool in_frustum(int16_t vc_x, int16_t vc_y);
void perspective_to_screen(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                           int16_t vc_z_bot, uint8_t *sx, uint8_t *sy_top,
                           uint8_t *sy_bot);

constexpr int16_t wall_top_z = 100;
constexpr int16_t wall_bot_z = 0;

void perspective_wall_move_to(uint16_t x, uint16_t y) {
  to_vc(x, y, &perspective_cur_vc_x, &perspective_cur_vc_y);
  if (in_frustum(perspective_cur_vc_x, perspective_cur_vc_y)) {
    int16_t vc_z_top = wall_top_z - player_z;
    int16_t vc_z_bot = wall_bot_z - player_z;

    uint8_t sx, sy_top, sy_bot;
    perspective_to_screen(perspective_cur_vc_x, perspective_cur_vc_y, vc_z_top,
                          vc_z_bot, &sx, &sy_top, &sy_bot);
#if 0
    uint16_t sx, sy;
    to_screen(perspective_cur_vc_x, perspective_cur_vc_y, &sx, &sy);
    line_move_to(sx, sy);
#endif
  }
}

void perspective_wall_draw_to(uint16_t x, uint16_t y) {}

void perspective_to_screen(int16_t vc_x, int16_t vc_y, int16_t vc_z_top,
                           int16_t vc_z_bot, uint8_t *sx, uint8_t *sy_top,
                           uint8_t *sy_bot) {}

bool in_frustum(int16_t vc_x, int16_t vc_y) {
  if (vc_x <= 0)
    return false;
  return abs(vc_y) <= vc_x;
}

void to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y) {
  int16_t tx = x - player_x;
  int16_t ty = y - player_y;
  // The player is facing up in the overhead view.
  uint16_t ang = PI_OVER_2 - player_ang;
  *vc_x = mul_cos(ang, tx) - mul_sin(ang, ty);
  *vc_y = mul_sin(ang, tx) + mul_cos(ang, ty);
}

bool on_screen(int16_t vc_x, int16_t vc_y) {
  int16_t x_bound = wc_width;
  int16_t y_bound = wc_height;
  return abs(vc_x) <= x_bound && abs(vc_y) <= y_bound;
}

bool line_on_screen(int16_t vc_x1, int16_t vc_y1, int16_t vc_x2,
                    int16_t vc_y2) {
  int16_t x_bound = wc_width;
  int16_t y_bound = wc_height;

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

void clip(int16_t vc_x, int16_t vc_y, int16_t *clip_vc_x, int16_t *clip_vc_y) {
  int16_t x_bound = wc_width;
  int16_t y_bound = wc_height;

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

constexpr uint16_t screen_guard = 5 * 256;

void overhead_to_screen(int16_t vc_x, int16_t vc_y, uint16_t *sx,
                        uint16_t *sy) {
  *sx = ((int32_t)vc_x * 256 * width / 2 * wc_width_recip >> 16) +
        width / 2 * 256 + screen_guard;
  *sy = height / 2 * 256 -
        ((int32_t)vc_y * 256 * width / 2 * wc_width_recip >> 16) + screen_guard;
}

extern "C" void __putchar(char c) { POKE(0x4018, c); }

uint16_t line_cur_x;
uint16_t line_cur_y;

void line_move_to(uint16_t x, uint16_t y) {
  line_cur_x = x;
  line_cur_y = y;
}

template <typename T> T rotl(T t, uint8_t amt) {
  return t << amt | t >> (sizeof(T) * 8 - amt);
}

volatile uint8_t and_mask;

void line_draw_to(uint8_t color, uint16_t to_x, uint16_t to_y) {
  int16_t dx = to_x - line_cur_x;
  int16_t dy = to_y - line_cur_y;

  bool x_major = abs(dx) >= abs(dy);
  if (x_major) {
    dy = (int32_t)dy * 256 / abs(dx);
    if (dx > 0)
      dx = 1 * 256;
    else if (dx < 0)
      dx = -1 * 256;
  } else {
    dx = (int32_t)dx * 256 / abs(dy);
    if (dy > 0)
      dy = 1 * 256;
    else if (dy < 0)
      dy = -1 * 256;
  }

  // Move onto the screen.
  uint16_t x = line_cur_x;
  uint16_t y = line_cur_y;
  while (x < screen_guard || y < screen_guard ||
         x >= width * 256 + screen_guard || y >= height * 256 + screen_guard) {
    if (x_major ? x / 256 == to_x / 256 : y / 256 == to_y / 256) {
      line_cur_x = to_x;
      line_cur_y = to_y;
      return;
    }
    x += dx;
    y += dy;
  }

  // Switch to unguarded screen coordinates.
  x -= screen_guard;
  to_x -= screen_guard;
  y -= screen_guard;
  to_y -= screen_guard;

  uint16_t offset = x / 256 / 2 * 30 + y / 256 / 2;
  while (x_major ? x / 256 != to_x / 256 : y / 256 != to_y / 256) {
    uint8_t shift = (x / 256 % 2 * 2 + y / 256 % 2) * 2;
    and_mask = rotl((uint8_t)0b11111100, shift);
    uint8_t or_mask = color << shift;
    fb_next[offset] &= and_mask;
    fb_next[offset] |= or_mask;

    if (dx < 0 ? x < -dx : x + dx >= width * 256)
      break;
    if (dy < 0 ? y < -dy : y + dy >= height * 256)
      break;

    if ((x + dx) / 256 / 2 > x / 256 / 2)
      offset += 30;
    else if ((x + dx) / 256 / 2 < x / 256 / 2)
      offset -= 30;
    if ((y + dy) / 256 / 2 > y / 256 / 2)
      offset++;
    else if ((y + dy) / 256 / 2 < y / 256 / 2)
      offset--;
    x += dx;
    y += dy;
  }
  line_cur_x = to_x + screen_guard;
  line_cur_y = to_y + screen_guard;
}

uint8_t cur_x;
uint8_t cur_y_top;
uint8_t cur_y_bot;

void wall_move_to(uint8_t x, uint8_t y_top, uint8_t y_bot) {
  cur_x = x;
  cur_y_top = y_top;
  cur_y_bot = y_bot;
}

template <bool x_odd>
void draw_vert_wall(uint8_t color, uint8_t *col, uint8_t y_top, uint8_t y_bot);

void wall_draw_to(uint8_t color, uint8_t x, uint8_t y_top, uint8_t y_bot) {
  uint8_t dx = x - cur_x;
  int8_t dy_top = y_top - cur_y_top;
  int8_t dy_bot = y_bot - cur_y_bot;
  // Values in 8.8 fixed point
  int m_top = dy_top * 256 / dx;
  int m_bot = dy_bot * 256 / dx;

  uint16_t y_top_fp = cur_y_top << 8;
  uint16_t y_bot_fp = cur_y_bot << 8;

  uint8_t *fb_col = &fb_next[cur_x / 2 * 30];
  for (uint8_t draw_x = cur_x; draw_x < x; ++draw_x) {
    if (draw_x & 1) {
      draw_vert_wall<true>(3, fb_col, 0, y_top_fp / 256);
      draw_vert_wall<true>(color, fb_col, y_top_fp / 256, y_bot_fp / 256);
      draw_vert_wall<true>(0, fb_col, y_bot_fp / 256, 61);
      fb_col += 30;
    } else {
      draw_vert_wall<false>(3, fb_col, 0, y_top_fp / 256);
      draw_vert_wall<false>(color, fb_col, y_top_fp / 256, y_bot_fp / 256);
      draw_vert_wall<false>(0, fb_col, y_bot_fp / 256, 61);
    }
    y_top_fp += m_top;
    y_bot_fp += m_bot;
  }
  cur_x = x;
  cur_y_top = y_top;
  cur_y_bot = y_bot;
}

template <bool x_odd>
void draw_vert_wall(uint8_t color, uint8_t *col, uint8_t y_top, uint8_t y_bot) {
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

volatile uint16_t max_updates_per_frame = 0;

#pragma clang section bss = ".prg_ram_0"
volatile uint8_t vram_buf[1025];
#pragma clang section bss = ""

volatile bool updating_vram;

void init_present() {
  uint16_t vbi = 0;
  // RTS
  vram_buf[vbi++] = 0x60;
  for (int i = 0; i < sizeof(vram_buf) / 16; i++) {
    vram_buf[vbi++] = 0xa9;
    vram_buf[vbi++] = 0;
    // STA PPUADDR
    vram_buf[vbi++] = 0x8d;
    vram_buf[vbi++] = 0x06;
    vram_buf[vbi++] = 0x20;
    // LDA #<vram
    vram_buf[vbi++] = 0xa9;
    vram_buf[vbi++] = 0;
    // STA PPUADDR
    vram_buf[vbi++] = 0x8d;
    vram_buf[vbi++] = 0x06;
    vram_buf[vbi++] = 0x20;
    // LDA #color
    vram_buf[vbi++] = 0xa9;
    vram_buf[vbi++] = 0;
    // STA PPUDATA
    vram_buf[vbi++] = 0x8d;
    vram_buf[vbi++] = 0x07;
    vram_buf[vbi++] = 0x20;
    // NOP
    vram_buf[vbi++] = 0x60;
  }
}

void present() {
  if (updating_vram)
    ppu_wait_nmi();

  static uint8_t x;
  static uint8_t *next_col;
  static uint8_t *cur_col;
  static uint16_t vram_col;
  if (!still_presenting) {
    x = 0;
    next_col = fb_next;
    cur_col = fb_cur;
    vram_col = NAMETABLE_A;
  }

  uint16_t vbi = 0;
  for (; x < 32; x++, next_col += 30, cur_col += 30, vram_col++) {
    for (uint8_t y = 0; y < 30; y++) {
      if (next_col[y] == cur_col[y])
        continue;

      uint16_t vram = vram_col + y * 32;
      if (vbi == sizeof(vram_buf) - 1) {
        still_presenting = true;
        ++y;
        goto done;
      }
      // Make any preexising RTS a NOP instead.
      vram_buf[vbi] = 0xea;
      vbi += 2;
      vram_buf[vbi] = vram >> 8;
      vbi += 5;
      vram_buf[vbi] = vram & 0xff;
      vbi += 5;
      vram_buf[vbi] = next_col[y];
      vbi += 4;
      cur_col[y] = next_col[y];
    }
  }
  still_presenting = false;
done:
  // Make the last NOP an RTS.
  vram_buf[vbi] = 0x60;
  updating_vram = true;
}

asm(".section .nmi.0,\"axR\"\n"
    "\tjsr update_vram\n");

extern volatile uint8_t VRAM_UPDATE;

extern "C" void update_vram() {
  if (!updating_vram)
    return;
  asm("jsr vram_buf");
  VRAM_UPDATE = 1;
  updating_vram = false;
}
