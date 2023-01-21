#include "screen.h"

#include <neslib.h>

bool still_presenting;

constexpr uint16_t max_cycles = 1024 + 512 + 256 + 32;

#pragma clang section bss = ".prg_ram_0"
uint8_t fb_cur[960];
uint8_t fb_next[960];
volatile uint8_t vram_buf[max_cycles];
#pragma clang section bss = ""

volatile bool updating_vram;

static const uint8_t ld_imm[] = {0xa9, 0xa2, 0xa0};
static const uint8_t st_abs[] = {0x8d, 0x8e, 0x8c};

__attribute__((section(".zp"))) uint8_t *present_x;
__attribute__((section(".zp"))) uint8_t *present_next_col;
__attribute__((section(".zp"))) uint8_t *present_cur_col;

__attribute__((noinline)) void present() {
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

  uint8_t reg_val[3] = {0};
  bool has_val[3] = {0};
  uint8_t mru, next_mru;
  bool consecutive = false;
  volatile uint8_t *vb = vram_buf;
  uint16_t cycles_remaining = max_cycles;
  for (; x < 32;
       x++, next_col += 30, cur_col += 30, vram_col++, consecutive = false) {
    uint16_t vram = vram_col;
    for (uint8_t y = 0; y < 30; y++, vram += 32) {
      if (next_col[y] == cur_col[y]) {
        consecutive = false;
        continue;
      }

      bool miss;
      uint8_t i = 0;
      const auto ldimm = [&](uint8_t val, bool update_lru) -> uint8_t {
        uint8_t reg;
        if (!has_val[0]) {
          reg = 0;
          miss = true;
          has_val[0] = true;
        } else if (val == reg_val[0]) {
          reg = 0;
          miss = false;
        } else if (!has_val[1]) {
          reg = 1;
          miss = true;
          has_val[1] = true;
        } else if (val == reg_val[1]) {
          reg = 1;
          miss = false;
        } else if (!has_val[2]) {
          reg = 2;
          miss = true;
          has_val[2] = true;
        } else if (val == reg_val[2]) {
          reg = 2;
          miss = false;
        } else {
          for (reg = 0; mru == reg || next_mru == reg; ++reg)
            ;
          miss = true;
        }

        if (miss) {
          reg_val[reg] = val;
          vb[i++] = ld_imm[reg];
          vb[i++] = val;
        }
        if (update_lru && mru != reg) {
          next_mru = mru;
          mru = reg;
        }
        return reg;
      };

      uint8_t reg;
      if (!consecutive) {
        if (cycles_remaining < 18) {
          still_presenting = true;
          goto done;
        }

        // LD #>vram
        reg = ldimm(vram >> 8, false);
        if (miss)
          cycles_remaining -= 2;
        // ST PPUADDR
        vb[i++] = st_abs[reg];
        vb[i++] = 0x06;
        vb[i++] = 0x20;
        // LD #<vram
        reg = ldimm(vram & 0xff, false);
        if (miss)
          cycles_remaining -= 2;
        // ST PPUADDR
        vb[i++] = st_abs[reg];
        vb[i++] = 0x06;
        vb[i++] = 0x20;
        consecutive = true;
        cycles_remaining -= 8;
      } else if (cycles_remaining < 6) {
          still_presenting = true;
          goto done;
      }
      reg = ldimm(next_col[y], true);
      if (miss)
        cycles_remaining -= 2;
      // ST PPUDATA
      vb[i++] = st_abs[reg];
      vb[i++] = 0x07;
      vb[i++] = 0x20;
      cur_col[y] = next_col[y];
      cycles_remaining -= 4;

      vb += i;
    }
  }
  still_presenting = false;
done:
  // RTS
  *vb = 0x60;
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
