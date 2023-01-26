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

extern Map test_map;
extern Map square_map;
extern Map outer_inner_map;

static Map *maps[] = {&test_map, &square_map, &outer_inner_map};
static uint8_t cur_map_idx = 0;

static bool overhead_view = true;

static void update();
static void idle(uint8_t last_present);

int main() {
  static const uint8_t bg_pal[16] = {0x16, 0x06, 0x07, 0x0f};
  static const uint8_t spr_pal[16] = {0x00, 0x00, 0x10, 0x30};
  ppu_off();
  set_mmc1_ctrl(0b01100);
  set_prg_bank(0);
  pal_bright(4);
  pal_bg(bg_pal);
  pal_spr(spr_pal);
  bank_spr(1);
  oam_spr(128 - 4, 120 - 4, 0, 0);
  ppu_on_all();

  load_map(test_map);
  uint8_t last_update = get_frame_count();
  uint8_t last_present = get_frame_count();
  while (true) {
    uint8_t cur_update = get_frame_count();
    for (; last_update != cur_update; ++last_update)
      update();
    if (!still_presenting) {
      if (overhead_view)
        overhead::render(*maps[cur_map_idx]);
      else
        perspective::render(*maps[cur_map_idx]);
    }
    idle(last_present);
    present();
    last_present = get_frame_count();
  }
}

__attribute__((noinline)) static void idle(uint8_t last_present) {
  while (get_frame_count() == last_present)
    ;
}

__attribute__((noinline)) static void update() {
  uint8_t pad_t = pad_trigger(0);
  uint8_t pad = pad_state(0);

  if (pad_t & PAD_START) {
    overhead_view = !overhead_view;
    if (overhead_view)
      oam_spr(128 - 4, 120 - 4, 0, 0);
    else
      oam_clear();
  }
  if (pad_t & PAD_SELECT) {
    if (++cur_map_idx == sizeof(maps) / sizeof(Map *))
      cur_map_idx = 0;
    load_map(*maps[cur_map_idx]);
  }
  if (overhead_view && pad & PAD_B) {
    if (pad & PAD_UP)
      overhead::scale_up();
    else if (pad & PAD_DOWN)
      overhead::scale_down();
  } else {
    if (pad & PAD_A) {
      if (pad & PAD_UP)
        player.fly_up();
      else if (pad & PAD_DOWN)
        player.fly_down();
      if (pad & PAD_LEFT)
        player.strafe_left();
      else if (pad & PAD_RIGHT)
        player.strafe_right();
    } else {
      if (pad & PAD_UP)
        player.forward();
      else if (pad & PAD_DOWN)
        player.backward();
      if (pad & PAD_LEFT)
        player.turn_left();
      else if (pad & PAD_RIGHT)
        player.turn_right();
    }
  }
#if 0
    if (!overhead_view && pad_t & PAD_B && player.z == 50)
      player.dz = 8;
    player.z += player.dz;
    player.dz -= 2;
    if (player.z < 50) {
      player.z = 50;
      player.dz = 0;
    }
#endif
}

extern "C" void __putchar(char c) { POKE(0x4018, c); }
