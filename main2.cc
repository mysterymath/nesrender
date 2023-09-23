#include <ines.h>
#include <nes.h>
#include <peekpoke.h>
#include <stdint.h>

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

constexpr u16 MMC1_CTRL = 0x8000;

constexpr u16 BG_PALS = 0x3f00;

int main() {
  static const uint8_t bg_pal[16] = {0x0f, 0x06, 0x16, 0x0c};
  mmc1_register_write(MMC1_CTRL, 0b01100);
  PPU.control = 0b00001000;
  ppu_set_addr(BG_PALS);
  for (u16 i = 0; i < sizeof(bg_pal); i++)
    PPU.vram.data = bg_pal[i];

  while (true)
    c = 0;
}
