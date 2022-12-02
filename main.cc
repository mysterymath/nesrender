#include <6502.h>
#include <bank.h>
#include <nes.h>
#include <nesdoug.h>
#include <neslib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "trig.h"

// Configure for SNROM MMC1 board.
asm(".globl __chr_rom_size\n"
    "__chr_rom_size = 8\n"
    ".globl __prg_ram_size\n"
    "__prg_ram_size = 8\n");

#pragma clang section bss = ".prg_ram_0"
char fb_cur[960];
char fb_next[960];
#pragma clang section bss = ""

char corner_x = 30;
char corner_y_top = 10;
char corner_y_bot = 40;
bool control_top;

bool still_presenting;

void render();
void present();

unsigned frame_count;

uint16_t player_x = 150;
uint16_t player_y = 150;
uint16_t player_ang = 0;

constexpr uint16_t speed = 10;
constexpr uint16_t ang_speed = 1000;

uint16_t scale = 500;

// As a 0:16 fixed point number
constexpr uint16_t scale_speed = 100;

int main() {
  static const char bg_pal[16] = {0x00, 0x11, 0x16, 0x1a};
  ppu_off();
  set_mirroring(MIRROR_VERTICAL);
  pal_bg_bright(4);
  pal_bg(bg_pal);
  ppu_on_bg();

  while (true) {
    ppu_wait_nmi();
    if (!still_presenting) {
      char pad_t = pad_trigger(0);
      char pad = pad_state(0);
#if 0
      if (pad & PAD_LEFT)
        player_x -= speed;
      else if (pad & PAD_RIGHT)
        player_x += speed;
#else
      if (pad & PAD_LEFT)
        player_ang += ang_speed;
      else if (pad & PAD_RIGHT)
        player_ang -= ang_speed;
#endif
      if (pad & PAD_A) {
        if (pad & PAD_UP)
          scale = (uint32_t)scale * 65536 / scale_speed;
        else if (pad & PAD_DOWN)
          scale = (uint32_t)scale * scale_speed / 65536;
      } else {
#if 0
        if (pad & PAD_UP)
          player_y -= speed;
        else if (pad & PAD_DOWN)
          player_y += speed;
#else
        int16_t vec_x = cosi(player_ang) * speed / 65536;
        int16_t vec_y = sine(player_ang) * speed / 65536;
        if (pad & PAD_UP) {
          player_x += vec_x;
          player_x += vec_y;
        } else if (pad & PAD_DOWN) {
          player_x -= vec_x;
          player_x -= vec_y;
        }
#endif
      }

#if 0
      if (pad & PAD_LEFT && corner_x >= 2)
        corner_x -= 2;
      else if (pad & PAD_RIGHT && corner_x <= 61)
        corner_x += 2;
      if (pad & PAD_UP) {
        if (control_top) {
          if (corner_y_top >= 2)
            corner_y_top -= 2;
        } else {
          if (corner_y_bot >= 2)
            corner_y_bot -= 2;
        }
      } else if (pad & PAD_DOWN) {
        if (control_top) {
          if (corner_y_top <= 57)
            corner_y_top += 2;
        } else {
          if (corner_y_bot <= 57)
            corner_y_bot += 2;
        }
      }
      if (pad_t & PAD_A)
        control_top = !control_top;
#endif
#if !NDEBUG
      frame_count++;
#endif
      render();
    }
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
  memset(fb_next, 0, sizeof(fb_next));
  overhead_wall_move_to(100, 100);
  overhead_wall_draw_to(100, 200);
  overhead_wall_draw_to(200, 200);
  overhead_wall_draw_to(200, 100);
  overhead_wall_draw_to(100, 100);
}

void project(uint16_t *x, uint16_t *y);

void overhead_wall_move_to(uint16_t x, uint16_t y) {
  project(&x, &y);
  line_move_to(x, y);
}
void overhead_wall_draw_to(uint16_t x, uint16_t y) {
  project(&x, &y);
  line_draw_to(3, x, y);
}

constexpr uint16_t width = 64;
constexpr uint16_t height = 60;

void project(uint16_t *x, uint16_t *y) {
  int16_t tx = *x - player_x;
  int16_t ty = *y - player_y;
#if 1
  uint16_t ang = 65536 - player_ang;
  int32_t c = cosi(ang);
  int32_t s = sine(ang);
  int16_t rx = c * tx / 65536 - s * ty / 65536;
  int16_t ry = s * tx / 65536 + c * ty / 65536;
#else
  int16_t rx = tx;
  int16_t ry = ty;
#endif
  int16_t sx = (int32_t)rx * 256 * width / scale;
  int16_t sy = -(int32_t)ry * 256 * width / scale;
  *x = sx + width * 256 / 2;
  *y = sy + height * 256 / 2;
}

uint16_t line_cur_x;
// uint8_t *line_cur_fb_x;
uint16_t line_cur_y;

void line_move_to(uint16_t x, uint16_t y) {
  line_cur_x = x;
  line_cur_y = y;
  // line_cur_fb_x = (uint8_t*)fb_next + (x >> 8) * 30;
}

template <typename T> T abs(T t) { return t < 0 ? -t : t; }
template <typename T> T rotl(T t, uint8_t amt) {
  return t << amt | t >> (sizeof(T) * 8 - amt);
}

volatile uint8_t and_mask;

void line_draw_to(uint8_t color, uint16_t to_x, uint16_t to_y) {
  int16_t dx = to_x - line_cur_x;
  int16_t dy = to_y - line_cur_y;

  if (abs(dx) >= abs(dy)) {
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
  while ((dx < 0 ? x > to_x : x < to_x) || (dy < 0 ? y > to_y : y < to_y)) {
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
volatile char vram_buf[1500];
#pragma clang section bss = ""

volatile bool vram_buf_ready;

void present() {
  unsigned vbi = 0;
#if !NDEBUG
  if (frame_count > 2 && !still_presenting) {
    unsigned num_updates = 0;
    unsigned offset = 0;
    for (char x = 0; x < 32; x++) {
      for (char y = 0; y < 30; y++) {
        if (fb_next[offset] != fb_b[offset])
          ++num_updates;
        offset++;
      }
    }
    if (num_updates > max_updates_per_frame)
      max_updates_per_frame = num_updates;
  }
#endif

  char *next = fb_next;
  char *cur = fb_cur;
  unsigned vram = NAMETABLE_A;

  still_presenting = false;
  vram_buf_ready = false;
  char cur_color = 0;
  for (char x = 0; x < 32; x++, vram -= 959) {
    for (char y = 0; y < 30; y++, next++, cur++, vram += 32) {
      if (*next == *cur) {
        continue;
      }

      if (vbi + 16 >= sizeof(vram_buf)) {
        still_presenting = true;
        goto done;
      }
      // LDA #>vram
      vram_buf[vbi++] = 0xa9;
      vram_buf[vbi++] = vram >> 8;
      // STA PPUADDR
      vram_buf[vbi++] = 0x8d;
      vram_buf[vbi++] = 0x06;
      vram_buf[vbi++] = 0x20;
      // LDA #<vram
      vram_buf[vbi++] = 0xa9;
      vram_buf[vbi++] = vram & 0xff;
      // STA PPUADDR
      vram_buf[vbi++] = 0x8d;
      vram_buf[vbi++] = 0x06;
      vram_buf[vbi++] = 0x20;
      // LDA #color
      vram_buf[vbi++] = 0xa9;
      vram_buf[vbi++] = *next;
      // STA PPUDATA
      vram_buf[vbi++] = 0x8d;
      vram_buf[vbi++] = 0x07;
      vram_buf[vbi++] = 0x20;
      *cur = *next;
    }
  }
done:
  // RTS
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
