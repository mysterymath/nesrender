#include <ines.h>
#include <nes.h>
#include <peekpoke.h>
#include <stdint.h>

// Configure for SNROM MMC1 board.
MAPPER_CHR_ROM_KB(8);
MAPPER_PRG_RAM_KB(8);

typedef uint16_t u16;
typedef uint8_t u8;

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
constexpr u16 bg_pals = 0x3f00;

constexpr u8 frame_buffer_stride = 2+3; // LDA imm, STA abs
extern u8 frame_buffer[frame_buffer_stride * 24 * 32 + 1];

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

// Fill the hud w/ a nice checkerboard pattern.
static void init_hud() {
  ppu_set_addr(0x2000 + 24*32);
  for (u16 i = 24*32; i < 30*32; i++)
    PPU.vram.data = 86;
}

int main() {
  init_framebuffer();
  init_hud();

  static const uint8_t bg_pal[16] = {0x0f, 0x06, 0x16, 0x0c};
  mmc1_register_write(mmc1_ctrl, 0b01100);
  PPU.control = 0b00001000;
  ppu_set_addr(bg_pals);
  for (u16 i = 0; i < sizeof(bg_pal); i++)
    PPU.vram.data = bg_pal[i];

  PPU.control = 0b10001000;
  PPU.mask = 0b0001110;

  while (true)
    c = 0;
}
