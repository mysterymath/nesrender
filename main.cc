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

#include "map.h"
#include "overhead.h"
#include "perspective.h"
#include "screen.h"

// Configure for SNROM MMC1 board.
asm(".globl __chr_rom_size\n"
    "__chr_rom_size = 8\n"
    ".globl __prg_ram_size\n"
    "__prg_ram_size = 8\n");

bool overhead_view = true;

int main() {
  static const uint8_t bg_pal[16] = {0x00, 0x11, 0x16, 0x1a};
  static const uint8_t spr_pal[16] = {0x00, 0x00, 0x10, 0x30};
  ppu_off();
  set_mmc1_ctrl(0b01100);
  set_prg_bank(0);
  pal_bright(4);
  pal_bg(bg_pal);
  pal_spr(spr_pal);
  bank_spr(1);
  ppu_on_all();

  oam_spr(128 - 4, 120 - 4, 0, 0);

  while (true) {
    uint8_t pad_t = pad_trigger(0);
    uint8_t pad = pad_state(0);

    if (pad_t & PAD_START) {
      overhead_view = !overhead_view;
      if (overhead_view)
        oam_spr(128 - 4, 120 - 4, 0, 0);
      else
        oam_clear();
    }
    if (overhead_view && pad & PAD_B) {
      if (pad & PAD_UP)
        overhead::scale_up();
      else if (pad & PAD_DOWN)
        overhead::scale_down();
    } else {
      if (pad & PAD_UP)
        player.forward();
      else if (pad & PAD_DOWN)
        player.backward();
      if (pad & PAD_A) {
        if (pad & PAD_LEFT)
          player.strafe_left();
        else if (pad & PAD_RIGHT)
          player.strafe_right();
      } else {
        if (pad & PAD_LEFT)
          player.turn_left();
        else if (pad & PAD_RIGHT)
          player.turn_right();
      }
    }

    if (!still_presenting) {
      if (overhead_view)
        overhead::render();
      else
        perspective::render();
    }
    present();
  }
}

extern "C" void __putchar(char c) { POKE(0x4018, c); }
