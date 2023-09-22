#include <ines.h>
#include <nes.h>
#include <peekpoke.h>
#include <stdint.h>

__attribute__((always_inline)) static inline void
mmc1_register_write(unsigned addr, char val) {
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

volatile uint8_t c;

constexpr unsigned MMC1_CTRL = 0x8000;

int main() {
  static const uint8_t bg_pal[16] = {0x0f, 0x06, 0x16, 0x0c};
  static const uint8_t spr_pal[16] = {0x00, 0x00, 0x10, 0x30};
  mmc1_register_write(MMC1_CTRL, 0b01100);
  while (true)
    c = 0;
}
