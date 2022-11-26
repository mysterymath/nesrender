#include <bank.h>
#include <nesdoug.h>
#include <neslib.h>
#include <string.h>

// Configure for SNROM MMC1 board.
asm(".globl __chr_rom_size\n"
    "__chr_rom_size = 8\n"
    ".globl __prg_ram_size\n"
    "__prg_ram_size = 8\n");

#pragma clang section bss = ".prg_ram_0"
char fb_a[960];
char fb_b[960];
char fb_next[960];
#pragma clang section bss = ""

void render();
void present();

char x = 1;
char y_top = 2;
char y_bot = 3;
bool control_top;

bool still_presenting;

int main() {
  static const char bg_pal[16] = {0x00, 0x11, 0x16, 0x1a};
  ppu_off();
  set_mirroring(MIRROR_VERTICAL);
  pal_bg_bright(4);
  pal_bg(bg_pal);
  scroll(0x100, 0);
  ppu_on_bg();

  while (true) {
    ppu_wait_nmi();
    set_vram_update(NULL);
    if (!still_presenting) {
      char pad_t = pad_trigger(0);
      char pad = pad_state(0);
      if (pad & PAD_LEFT)
        x = x ? x - 1 : 31;
      else if (pad & PAD_RIGHT)
        x = x == 31 ? 0 : x + 1;
      if (pad & PAD_UP) {
        if (control_top)
          y_top = y_top ? y_top - 1 : 29;
        else
          y_bot = y_bot ? y_bot - 1 : 29;
      } else if (pad & PAD_DOWN) {
        if (control_top)
          y_top = y_top == 29 ? 0 : y_top + 1;
        else
          y_bot = y_bot == 29 ? 0 : y_bot + 1;
      }
      if (pad_t & PAD_A)
        control_top = !control_top;
    }
    gray_line();
    render();
    gray_line();
    present();
    gray_line();
  }
}

void draw_vert_line(char color, char x, char y_top, char y_bot) {
  memset(fb_next + x * 30 + y_top, color, y_bot - y_top + 1);
}

void render() {
  memset(fb_next, 0, sizeof(fb_next));
  if (y_top <= y_bot)
    draw_vert_line(1, x, y_top, y_bot);
}

void present() {
  char vram_buf_idx = 0;
  static char vram_buf[40];
  static bool present_to_nt_b;

  unsigned offset = 0;
  still_presenting = false;
  for (char x = 0; x < 32; x++) {
    for (char y = 0; y < 30; y++) {
      if (vram_buf_idx + 4 >= sizeof(vram_buf)) {
        still_presenting = true;
        goto done;
      }
      if (present_to_nt_b) {
        if (fb_next[offset] != fb_b[offset]) {
          unsigned addr = NTADR_B(x, y);
          vram_buf[vram_buf_idx++] = addr >> 8;
          vram_buf[vram_buf_idx++] = addr & 0xff;
          vram_buf[vram_buf_idx++] = fb_next[offset];
          fb_b[offset] = fb_next[offset];
        }
      } else {
        if (fb_next[offset] != fb_a[offset]) {
          unsigned addr = NTADR_A(x, y);
          vram_buf[vram_buf_idx++] = addr >> 8;
          vram_buf[vram_buf_idx++] = addr & 0xff;
          vram_buf[vram_buf_idx++] = fb_next[offset];
          fb_a[offset] = fb_next[offset];
        }
      }
      ++offset;
    }
  }
done:
  vram_buf[vram_buf_idx] = NT_UPD_EOF;
  set_vram_update(vram_buf);
  if (!still_presenting) {
    scroll(present_to_nt_b ? 0x100 : 0, 0);
    present_to_nt_b = !present_to_nt_b;
  }
}
