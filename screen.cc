#include "screen.h"

#include <neslib.h>

bool still_presenting;

#pragma clang section bss = ".prg_ram_0"
uint8_t fb_cur[960];
uint8_t fb_next[960];
volatile uint8_t vram_buf[1025];
#pragma clang section bss = ""

volatile bool updating_vram;

__attribute__((constructor)) static void init() {
  uint16_t vbi = 0;
  // RTS
  vram_buf[vbi++] = 0x60;
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
    vram_buf[vbi++] = 0x60;
  }
}

void present() {
  static uint8_t x;
  static uint8_t *next_col;
  static uint8_t *cur_col;
  static uint16_t vram_col;
  if (!still_presenting) {
    x = 0;
    next_col = fb_next;
    cur_col = fb_cur;
    vram_col = NAMETABLE_A;
  }

  uint16_t vbi = 0;
  for (; x < 32; x++, next_col += 30, cur_col += 30, vram_col++) {
    uint16_t vram = vram_col;
    for (uint8_t y = 0; y < 30; y++, vram += 32) {
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
  still_presenting = false;
done:
  // Make the last NOP an RTS.
  vram_buf[vbi] = 0x60;
  updating_vram = true;
}

asm(".section .nmi.0,\"axR\"\n"
    "\tjsr update_vram\n");

extern volatile uint8_t VRAM_UPDATE;

extern "C" void update_vram() {
  if (!updating_vram)
    return;
  asm("jsr vram_buf");
  VRAM_UPDATE = 1;
  updating_vram = false;
}
