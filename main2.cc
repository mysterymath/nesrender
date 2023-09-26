#include <ines.h>
#include <nes.h>
#include <peekpoke.h>
#include <stdint.h>

// Configure for SNROM MMC1 board.
MAPPER_CHR_ROM_KB(8);
MAPPER_PRG_RAM_KB(8);

typedef uint16_t u16;
typedef uint8_t u8;

constexpr u16 ppu_bg_pals = 0x3f00;
constexpr u16 ppu_spr_pals = 0x3f11;

struct oam {
  u8 y;
  u8 tile;
  u8 attrs;
  u8 x;
};

alignas(256) oam oam_buf[64];

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

constexpr u8 fb_height_tiles = 22;
constexpr u8 fb_width_tiles = 32;

constexpr u8 frame_buffer_stride = 2 + 3; // LDA imm, STA abs
extern volatile u8
    frame_buffer[frame_buffer_stride * fb_height_tiles * fb_width_tiles + 1];

/* 16-bit xorshift PRNG */
unsigned xorshift() {
  static unsigned x = 1;
  x ^= x << 7;
  x ^= x >> 9;
  x ^= x << 8;
  return x;
}

static void init_framebuffer() {
  u16 tile = 0;
  for (u16 i = 0; i < sizeof(frame_buffer);) {
    static constexpr u8 lda_imm = 0xa9;
    frame_buffer[i++] = lda_imm;
    frame_buffer[i++] = tile++;

    static constexpr u8 sta_abs = 0x8d;
    frame_buffer[i++] = sta_abs;
    frame_buffer[i++] = reinterpret_cast<u16>(&PPU.vram.data) & 0xff;
    frame_buffer[i++] = reinterpret_cast<u16>(&PPU.vram.data) >> 8;
  }
  static constexpr u8 rts = 0x60;
  frame_buffer[sizeof(frame_buffer) - 1] = rts; // RTS
}

// Fill the hud w/ a nice checkerboard pattern and zero the attribute table.
static void init_nametable_remainder() {
  ppu_set_addr(0x2000 + fb_height_tiles * fb_width_tiles);
  u16 i;
  for (i = fb_height_tiles * fb_width_tiles; i < 30 * 32; i++)
    PPU.vram.data = 86;
  // Zero the attribute table.
  for (; i < 1024; i++)
    PPU.vram.data = 0;
}

[[clang::noinline]] static void randomize_tiles() {
  for (u16 idx = 1; idx < sizeof(frame_buffer); idx += 5)
    frame_buffer[idx] = xorshift();
}

static void init_sprites() {
  for (u8 i = 0; i < 8; ++i) {
    oam_buf[i].tile = 10; // Arrow
    oam_buf[i].x = xorshift();
    oam_buf[i].y = xorshift();
  }
}

int main() {
  init_framebuffer();
  init_nametable_remainder();
  init_sprites();

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

  PPU.control = 0b10001000;
  PPU.mask = 0b0011110;

  while (true)
    randomize_tiles();
}
