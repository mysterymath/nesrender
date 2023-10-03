#include <ines.h>
#include <nes.h>
#include <neslib.h>
#include <peekpoke.h>
#include <soa.h>
#include <stdio.h>

#include "framebuffer-constants.h"
#include "framebuffer.h"
#include "logo.h"
#include "types.h"

// Configure for SNROM MMC1 board.
MAPPER_CHR_ROM_KB(8);
MAPPER_PRG_RAM_KB(8);

__attribute__((section(".zp.bss"))) volatile u8 frame_count;

/* 16-bit xorshift PRNG */
u16 xorshift() {
  static u16 x = 1;
  x ^= x << 7;
  x ^= x >> 9;
  x ^= x << 8;
  return x;
}

constexpr u16 ppu_bg_pals = 0x3f00;
constexpr u16 ppu_spr_pals = 0x3f11;

struct OAM {
  u8 y;
  u8 tile;
  u8 attrs;
  u8 x;
};

alignas(256) OAM oam_buf[64];

__attribute__((always_inline)) static inline void mmc1_register_write(u16 addr,
                                                                      u8 val) {
  POKE(addr, val);
  val >>= 1;
  POKE(addr, val);
  val >>= 1;
  POKE(addr, val);
  val >>= 1;
  POKE(addr, val);
  val >>= 1;
  POKE(addr, val);
}

static void ppu_set_addr(u16 addr) {
  (void)PPU.status;
  PPU.vram.address = addr >> 8;
  PPU.vram.address = addr & 0xff;
}

volatile uint8_t c;

constexpr u16 mmc1_ctrl = 0x8000;

// The first row is corrupted, so black it out.
static void init_first_row() {
  ppu_set_addr(0x2000);
  for (u8 i = 0; i < FRAMEBUFFER_WIDTH_TILES; i++)
    PPU.vram.data = 0;
}

static void init_framebuffer() {
  u16 tile = 0;
  for (u16 i = 0; i < sizeof(framebuffer);) {
    static constexpr u8 lda_imm = 0xa9;
    framebuffer[i++] = lda_imm;
    framebuffer[i++] = tile++;

    static constexpr u8 sta_abs = 0x8d;
    framebuffer[i++] = sta_abs;
    framebuffer[i++] = reinterpret_cast<u16>(&PPU.vram.data) & 0xff;
    framebuffer[i++] = reinterpret_cast<u16>(&PPU.vram.data) >> 8;
  }
  static constexpr u8 rts = 0x60;
  framebuffer[sizeof(framebuffer) - 1] = rts; // RTS
}

// Fill the hud w/ a nice checkerboard pattern and zero the attribute table.
static void init_nametable_remainder() {
  ppu_set_addr(0x2000 + FRAMEBUFFER_HEIGHT_TILES * FRAMEBUFFER_WIDTH_TILES);
  u16 i;
  for (i = FRAMEBUFFER_HEIGHT_TILES * FRAMEBUFFER_WIDTH_TILES; i < 30 * 32; i++)
    PPU.vram.data = 86;
  // Zero the attribute table.
  for (; i < 1024; i++)
    PPU.vram.data = 0;
}

void latch_joypads() {
  JOYPAD[0] = 1;
  JOYPAD[0] = 0;
}

u8 joy1_prev;
u8 joy1;

void read_joypad1() {
  joy1_prev = joy1;
  joy1 = 0;
  for (u8 i = 0; i < 8; i++) {
    joy1 >>= 1;
    joy1 |= JOYPAD[0] << 7;
  }
}

u16 x_scale = 0x100;
u16 x_offset = 0;

enum Select {
  X_SCALE,
  X_OFFSET,
  SELECT_SIZE,
} select;

u16 *select_ptrs[] = {&x_scale, &x_offset};
u16 select_wraps[] = {65535, u16(logo.width << 8)};
u8 select_increments[] = {10, 64};

void update() {
  latch_joypads();
  read_joypad1();
  u16 *ptr = select_ptrs[select];
  u16 wrap = select_wraps[select];
  u8 increment = select_increments[select];
  if (joy1 & JOY_RIGHT_MASK) {
    if (*ptr < increment)
      *ptr += wrap;
    *ptr -= increment;
  }
  if (joy1 & JOY_LEFT_MASK) {
    *ptr += increment;
    if (*ptr >= wrap)
      *ptr -= wrap;
  }
  if (joy1 & ~joy1_prev & JOY_BTN_A_MASK)
    select = Select((select + 1) % SELECT_SIZE);
}

int main() {
  init_first_row();
  init_framebuffer();
  init_nametable_remainder();

  mmc1_register_write(mmc1_ctrl, 0b01100);
  PPU.control = 0b00001000;

  ppu_set_addr(ppu_bg_pals);
  static const uint8_t bg_pals[16] = {0x0f, 0x06, 0x16, 0x26};
  for (u16 i = 0; i < sizeof(bg_pals); i++)
    PPU.vram.data = bg_pals[i];

  ppu_set_addr(ppu_spr_pals);
  static const uint8_t spr_pals[15] = {0x00, 0x10, 0x30};
  for (u16 i = 0; i < sizeof(spr_pals); i++)
    PPU.vram.data = spr_pals[i];

  // FPS counter
  oam_buf[0].x = 8;
  oam_buf[0].y = 24;
  oam_buf[1].x = 16;
  oam_buf[1].y = 24;

  PPU.control = 0b10001000;
  PPU.mask = 0b0011110;

  u8 last_update = frame_count;
  u8 last_render = frame_count;
  while (true) {
    u8 cur_frame = frame_count;

    // Handle wraparound if any.
    if (cur_frame < last_update)
      while (!__builtin_add_overflow(last_update, 2, &last_update))
        update();

    while (last_update < cur_frame) {
      // Even if cur_frame didn't wrap around, last_update may wrap around as it
      // approaches a cur_frame near 255.
      update();
      if (__builtin_add_overflow(last_update, 2, &last_update))
        break;
    }

    u8 fps = 60 / (cur_frame < last_render ? cur_frame + 256 - last_render
                                           : cur_frame - last_render);
    oam_buf[0].tile = fps / 10;
    oam_buf[1].tile = fps % 10;

    u16 x_pos = x_offset + x_scale / 2;
    if (x_pos >= logo.width << 8)
      x_pos -= logo.width << 8;

    u8 x_col;
    const TextureColumn *texture_column = logo.columns;
    for (x_col = 0; x_col != x_pos >> 8; ++x_col)
      texture_column = texture_column->next();

    const auto advance_column = [&] {
      x_pos += x_scale;
      if (x_pos >= logo.width << 8) {
        x_pos -= logo.width << 8;
        texture_column = logo.columns;
        x_col = 0;
      }
      for (; x_col != x_pos >> 8; ++x_col)
        texture_column = texture_column->next();
    };
    for (u8 column_offset = 0;
         column_offset < FRAMEBUFFER_WIDTH_TILES * framebuffer_stride;
         column_offset += framebuffer_stride) {
      texture_column->render(/*left=*/true);
      advance_column();
      texture_column->render(/*left=*/false);
      render_framebuffer_columns(column_offset);
      advance_column();
    }
    last_render = cur_frame;
  }
}

extern "C" void __putchar(char c) { POKE(0x4018, c); }