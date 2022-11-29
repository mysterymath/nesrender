#include <bank.h>
#include <nes.h>
#include <nesdoug.h>
#include <neslib.h>
#include <stdbool.h>
#include <string.h>

// Configure for SNROM MMC1 board.
asm(".globl __chr_rom_size\n"
    "__chr_rom_size = 8\n"
    ".globl __prg_ram_size\n"
    "__prg_ram_size = 8\n");

#define DOUBLE_BUFFER false

#pragma clang section bss = ".prg_ram_0"
char fb_a[960];
char fb_b[960];
char fb_next[960];
#pragma clang section bss = ""

char corner_x = 15;
char corner_y_top = 5;
char corner_y_bot = 20;
bool control_top;

bool still_presenting;

void render();
void present();

unsigned frame_count;

int main() {
  static const char bg_pal[16] = {0x00, 0x11, 0x16, 0x1a};
  ppu_off();
  set_mirroring(MIRROR_VERTICAL);
  pal_bg_bright(4);
  pal_bg(bg_pal);
  if (DOUBLE_BUFFER)
    scroll(0x100, 0);
  ppu_on_bg();

  while (true) {
    ppu_wait_nmi();
    if (!still_presenting) {
      char pad_t = pad_trigger(0);
      char pad = pad_state(0);
      if (pad & PAD_LEFT)
        corner_x = corner_x ? corner_x - 1 : 31;
      else if (pad & PAD_RIGHT)
        corner_x = corner_x == 31 ? 0 : corner_x + 1;
      if (pad & PAD_UP) {
        if (control_top)
          corner_y_top = corner_y_top ? corner_y_top - 1 : 29;
        else
          corner_y_bot = corner_y_bot ? corner_y_bot - 1 : 29;
      } else if (pad & PAD_DOWN) {
        if (control_top)
          corner_y_top = corner_y_top == 29 ? 0 : corner_y_top + 1;
        else
          corner_y_bot = corner_y_bot == 29 ? 0 : corner_y_bot + 1;
      }
      if (pad_t & PAD_A)
        control_top = !control_top;
#if !NDEBUG
      frame_count++;
#endif
    }
    render();
    present();
  }
}

void wall_move_to(char x, char y_top, char y_bot);
void wall_draw_to(char color, char x, char y_top, char y_bot);

void render() {
  wall_move_to(0, 0, 29);
  wall_draw_to(1, corner_x, corner_y_top, corner_y_bot);
  wall_draw_to(2, 33, 0, 29);
}

volatile unsigned max_updates_per_frame = 0;

#pragma clang section bss = ".prg_ram_0"
volatile char vram_buf[1500];
#pragma clang section bss = ""

volatile bool vram_buf_ready;

__attribute__((noinline)) void present() {
  unsigned vbi = 0;
  static bool present_to_nt_b;

#if !NDEBUG
  if (frame_count > 2 && !still_presenting) {
    unsigned num_updates = 0;
    unsigned offset = 0;
    for (char x = 0; x < 32; x++) {
      for (char y = 0; y < 30; y++) {
        if (present_to_nt_b) {
          if (fb_next[offset] != fb_b[offset])
            ++num_updates;
        } else {
          if (fb_next[offset] != fb_a[offset])
            ++num_updates;
        }
        offset++;
      }
    }
    if (num_updates > max_updates_per_frame)
      max_updates_per_frame = num_updates;
  }
#endif

  char *next = fb_next;
  char *prev = DOUBLE_BUFFER && present_to_nt_b ? fb_b : fb_a;
  unsigned vram = DOUBLE_BUFFER && present_to_nt_b ? NAMETABLE_B : NAMETABLE_A;

  still_presenting = false;
  vram_buf_ready = false;
  char cur_color = 0;
  for (char x = 0; x < 32; x++, vram -= 959) {
    for (char y = 0; y < 30; y++, next++, prev++, vram += 32) {
      if (*next == *prev) {
        continue;
      }

      if (vbi + 16 >= sizeof(vram_buf)) {
        still_presenting = true;
        goto done;
      }
      // LDA #>vram
      vram_buf[vbi++] = 0xa9;
      vram_buf[vbi++] = vram >> 8;
      // STA PPUADDR
      vram_buf[vbi++] = 0x8d;
      vram_buf[vbi++] = 0x06;
      vram_buf[vbi++] = 0x20;
      // LDA #<vram
      vram_buf[vbi++] = 0xa9;
      vram_buf[vbi++] = vram & 0xff;
      // STA PPUADDR
      vram_buf[vbi++] = 0x8d;
      vram_buf[vbi++] = 0x06;
      vram_buf[vbi++] = 0x20;
      // LDA #color
      vram_buf[vbi++] = 0xa9;
      vram_buf[vbi++] = *next;
      // STA PPUDATA
      vram_buf[vbi++] = 0x8d;
      vram_buf[vbi++] = 0x07;
      vram_buf[vbi++] = 0x20;
      *prev = *next;
    }
  }
done:
  // RTS
  vram_buf[vbi] = 0x60;
  vram_buf_ready = true;
  if (!still_presenting && DOUBLE_BUFFER) {
    scroll(present_to_nt_b ? 0x100 : 0, 0);
    present_to_nt_b = !present_to_nt_b;
  }
}

asm(".section .nmi.0,\"axR\"\n"
    "\tjsr update_vram\n");

extern volatile char VRAM_UPDATE;

void update_vram() {
  if (!vram_buf_ready)
    return;
  asm("jsr vram_buf");
  VRAM_UPDATE = 1;
}

char cur_x;
char cur_y_top;
char cur_y_bot;

void wall_move_to(char x, char y_top, char y_bot) {
  cur_x = x;
  cur_y_top = y_top;
  cur_y_bot = y_bot;
}

void wall_draw_to(char color, char x, char y_top, char y_bot) {
  char dx = x - cur_x;
  signed char dy_top = y_top - cur_y_top;
  signed char dy_bot = y_bot - cur_y_bot;
  // Values in 8.8 fixed point
  int m_top = dy_top * 256 / dx;
  int m_bot = dy_bot * 256 / dx;

  unsigned y_top_fp = cur_y_top << 8;
  unsigned y_bot_fp = cur_y_bot << 8;

  char *fb_col = &fb_next[cur_x * 30];
  for (char draw_x = cur_x; draw_x < x; ++draw_x) {
    for (char y = 0; y < 30; ++y) {
      if (y < y_top_fp >> 8)
        fb_col[y] = 3;
      else if (y <= y_bot_fp >> 8)
        fb_col[y] = color;
      else
        fb_col[y] = 0;
    }
    fb_col += 30;
    y_top_fp += m_top;
    y_bot_fp += m_bot;
  }
  cur_x = x;
  cur_y_top = y_top;
  cur_y_bot = y_bot;
}
