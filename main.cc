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

char corner_x = 30;
char corner_y_top = 10;
char corner_y_bot = 40;
bool control_top;

constexpr uint16_t width = 64;
constexpr uint16_t height = 60;

uint16_t wc_width = 100;
uint16_t wc_width_recip = (uint32_t)65536 / wc_width;
uint16_t wc_height = wc_width * height / width;

// Percent
constexpr uint16_t scale_speed = 10;

uint16_t player_x = 150;
uint16_t player_y = 150;
uint16_t player_ang = 0;

constexpr uint16_t speed = 10;
constexpr uint16_t ang_speed = 2048;

bool still_presenting;

void render();
void init_present();
void present();

int main() {
  static const char bg_pal[16] = {0x00, 0x11, 0x16, 0x1a};
  static const char spr_pal[16] = {0x00, 0x00, 0x10, 0x30};
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
    ppu_wait_nmi();

    char pad_t = pad_trigger(0);
    char pad = pad_state(0);
    if (pad & PAD_B) {
      if (pad & PAD_UP) {
        wc_width = (uint32_t)wc_width * (100 - scale_speed) / 100;
        wc_width_recip = (uint32_t)wc_width_recip * (100 + scale_speed) / 100;
        wc_height = wc_width * height / width;
      } else if (pad & PAD_DOWN) {
        wc_width = (uint32_t)wc_width * (100 + scale_speed) / 100;
        wc_width_recip = (uint32_t)wc_width_recip * (100 - scale_speed) / 100;
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

    if (!still_presenting)
      render();
    present();
  }
}

// Values are 8:8 fixed point screen pixels.
void line_move_to(uint16_t x, uint16_t y);
void line_draw_to(uint8_t color, uint16_t x, uint16_t y);

void wall_move_to(char x, char y_top, char y_bot);
void wall_draw_to(char color, char x, char y_top, char y_bot);

void overhead_wall_move_to(uint16_t x, uint16_t y);
void overhead_wall_draw_to(uint16_t x, uint16_t y);

#pragma clang section bss = ".prg_ram_0"
char fb_cur[960];
char fb_next[960];
#pragma clang section bss = ""

__attribute__((no_builtin("memset"))) void clear_screen() {
  for (int i = 0; i < 256; i++) {
    fb_next[i] = 0;
    (fb_next + 256)[i] = 0;
    (fb_next + 512)[i] = 0;
  }
  for (char i = 0; i < 192; i++)
    (fb_next + 768)[i] = 0;
}

void render() {
#if 0
  wall_move_to(0, 0, 59);
  wall_draw_to(1, corner_x, corner_y_top, corner_y_bot);
  wall_draw_to(2, 65, 0, 59);
#elif 0
  memset(fb_next, 0, sizeof(fb_next));
  line_move_to(128, 128);
  line_draw_to(1, corner_x << 8 | 128, corner_y_bot << 8 | 128);
#endif
  clear_screen();
  overhead_wall_move_to(100, 100);
  overhead_wall_draw_to(100, 200);
  overhead_wall_draw_to(200, 200);
  overhead_wall_draw_to(200, 100);
  overhead_wall_draw_to(100, 100);
}

void to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y);
bool on_screen(int16_t vc_x, int16_t vc_y);
bool line_on_screen(int16_t vc_x1, int16_t vc_y1, int16_t vc_x2, int16_t vc_y2);
void clip(int16_t vc_x, int16_t vc_y, int16_t *clip_vc_x, int16_t *clip_vc_y);
void to_screen(int16_t vc_x, int16_t vc_y, int16_t *sx, int16_t *sy);

int16_t cur_vc_x;
int16_t cur_vc_y;

template <typename T> T abs(T t) { return t < 0 ? -t : t; }

void overhead_wall_move_to(uint16_t x, uint16_t y) {
  to_vc(x, y, &cur_vc_x, &cur_vc_y);
  if (on_screen(cur_vc_x, cur_vc_y)) {
    int16_t sx, sy;
    to_screen(cur_vc_x, cur_vc_y, &sx, &sy);
    line_move_to(sx, sy);
  }
}

void overhead_wall_draw_to(uint16_t x, uint16_t y) {
  int16_t vc_x, vc_y;
  to_vc(x, y, &vc_x, &vc_y);

  if (!line_on_screen(vc_x, vc_y, cur_vc_x, cur_vc_y)) {
    cur_vc_x = vc_x;
    cur_vc_y = vc_y;
    return;
  }

  int16_t unclipped_vc_x = vc_x;
  int16_t unclipped_vc_y = vc_y;

  if (!on_screen(cur_vc_x, cur_vc_y)) {
    clip(vc_x, vc_y, &cur_vc_x, &cur_vc_y);
    int16_t sx, sy;
    to_screen(cur_vc_x, cur_vc_y, &sx, &sy);
    line_move_to(sx, sy);
  }
  if (!on_screen(vc_x, vc_y))
    clip(cur_vc_x, cur_vc_y, &vc_x, &vc_y);

  cur_vc_x = unclipped_vc_x;
  cur_vc_y = unclipped_vc_y;

  int16_t sx, sy;
  to_screen(vc_x, vc_y, &sx, &sy);
  line_draw_to(3, sx, sy);
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

void to_screen(int16_t vc_x, int16_t vc_y, int16_t *sx, int16_t *sy) {
  *sx = ((int32_t)vc_x * 256 * width / 2 * wc_width_recip >> 16) +
        width / 2 * 256;
  *sy = height / 2 * 256 -
        ((int32_t)vc_y * 256 * width / 2 * wc_width_recip >> 16);
}

extern "C" void __putchar(char c) { POKE(0x4018, c); }

uint16_t line_cur_x;
// uint8_t *line_cur_fb_x;
uint16_t line_cur_y;

void line_move_to(uint16_t x, uint16_t y) {
  line_cur_x = x;
  line_cur_y = y;
  // line_cur_fb_x = (uint8_t*)fb_next + (x >> 8) * 30;
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

  uint16_t x = line_cur_x;
  uint16_t y = line_cur_y;
  while (x_major ? x / 256 != to_x / 256 : y / 256 != to_y / 256) {
    uint8_t shift = (x / 256 % 2 * 2 + y / 256 % 2) * 2;
    and_mask = rotl((uint8_t)0b11111100, shift);
    uint8_t or_mask = color << shift;
    uint16_t offset = x / 256 / 2 * 30 + y / 256 / 2;
    fb_next[offset] &= and_mask;
    fb_next[offset] |= or_mask;
    x += dx;
    y += dy;
  }
  line_cur_x = to_x;
  line_cur_y = to_y;
}

char cur_x;
char cur_y_top;
char cur_y_bot;

void wall_move_to(char x, char y_top, char y_bot) {
  cur_x = x;
  cur_y_top = y_top;
  cur_y_bot = y_bot;
}

template <bool x_odd>
void draw_vert_wall(char color, char *col, char y_top, char y_bot);

void wall_draw_to(char color, char x, char y_top, char y_bot) {
  char dx = x - cur_x;
  signed char dy_top = y_top - cur_y_top;
  signed char dy_bot = y_bot - cur_y_bot;
  // Values in 8.8 fixed point
  int m_top = dy_top * 256 / dx;
  int m_bot = dy_bot * 256 / dx;

  unsigned y_top_fp = cur_y_top << 8;
  unsigned y_bot_fp = cur_y_bot << 8;

  char *fb_col = &fb_next[cur_x / 2 * 30];
  for (char draw_x = cur_x; draw_x < x; ++draw_x) {
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
void draw_vert_wall(char color, char *col, char y_top, char y_bot) {
  if (y_top >= y_bot)
    return;
  char i = y_top / 2;
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

volatile unsigned max_updates_per_frame = 0;

#pragma clang section bss = ".prg_ram_0"
volatile char vram_buf[1025];
#pragma clang section bss = ""

volatile bool vram_buf_ready;

void init_present() {
  unsigned vbi = 0;
  // NOP
  vram_buf[vbi++] = 0xea;
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
    vram_buf[vbi++] = 0xea;
  }
}

void present() {
  unsigned vbi = 0;

  char *next_col = fb_next;
  char *cur_col = fb_cur;
  unsigned vram = NAMETABLE_A;

  still_presenting = false;
  vram_buf_ready = false;
  char cur_color = 0;
  for (char x = 0; x < 32; x++, vram -= 959, next_col += 30, cur_col += 30) {
    for (char y = 0; y < 30; y++, vram += 32) {
      if (next_col[y] == cur_col[y])
        continue;

      if (vbi == sizeof(vram_buf) - 1) {
        still_presenting = true;
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
done:
  // Make the last NOP an RTS.
  vram_buf[vbi] = 0x60;
  vram_buf_ready = true;
}

asm(".section .nmi.0,\"axR\"\n"
    "\tjsr update_vram\n");

extern volatile char VRAM_UPDATE;

extern "C" void update_vram() {
  if (!vram_buf_ready)
    return;
  asm("jsr vram_buf");
  VRAM_UPDATE = 1;
}
