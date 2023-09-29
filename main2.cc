#include <ines.h>
#include <nes.h>
#include <neslib.h>
#include <peekpoke.h>
#include <soa.h>

#include "framebuffer-constants.h"
#include "framebuffer.h"
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

void update() {
  // TODO
}

void randomize_span_buffer() {
  span_buffer.clear();
  u8 offset = 0;
  while (true) {
    u8 color = xorshift() % 4;
    u8 len = xorshift() % 32;
    bool done = false;
    if (offset + len >= FRAMEBUFFER_HEIGHT) {
      len = FRAMEBUFFER_HEIGHT - offset;
      done = true;
    }
    span_buffer.push_back({color, len});
    if (done)
      return;
    offset += len;
  }
}

int main() {
  init_first_row();
  init_framebuffer();
  init_nametable_remainder();

  mmc1_register_write(mmc1_ctrl, 0b01100);
  PPU.control = 0b00001000;

  ppu_set_addr(ppu_bg_pals);
  static const uint8_t bg_pals[16] = {0x0f, 0x06, 0x16, 0x0c};
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

  uint8_t last_update = frame_count;
  uint8_t last_render = frame_count;
  while (true) {
    uint8_t cur_frame = frame_count;

    // Handle wraparound if any.
    if (cur_frame < last_update) {
      while (!__builtin_add_overflow(last_update, 2, &last_update))
        ;
    }

    if (last_update >= cur_frame)
      continue;

    for (; last_update < cur_frame; last_update += 2)
      update();

    uint8_t fps = 60 / (cur_frame < last_render ? cur_frame + 256 - last_render
                                                : cur_frame - last_render);
    oam_buf[0].tile = fps / 10;
    oam_buf[1].tile = fps % 10;

    for (u8 column_offset = 0;
         column_offset < FRAMEBUFFER_WIDTH_TILES * framebuffer_stride;
         column_offset += framebuffer_stride) {
      randomize_span_buffer();
      render_span_buffer_left();
      randomize_span_buffer();
      render_span_buffer_right();
      render_framebuffer_columns(column_offset);
    }
    last_render = cur_frame;
  }
}
