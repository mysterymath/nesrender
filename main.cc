#include <nesdoug.h>
#include <neslib.h>

#include <string.h>

// Configure for SNROM MMC1 board.
asm (
  ".globl __chr_rom_size\n"
  "__chr_rom_size = 8\n"
  ".globl __prg_ram_size\n"
  "__prg_ram_size = 8\n"
);

void render();

char x = 1;
char y_top = 2;
char y_bot = 3;
bool control_top;

char skip_to_line;
bool unfinished;
volatile bool render_to_nt_b;

int main() {
  static const char bg_pal[16] = {0x00, 0x11, 0x16, 0x1a};
  ppu_off();
  pal_bg_bright(4);
  pal_bg(bg_pal);
  ppu_on_bg();

  while (true) {
    ppu_wait_nmi();
    set_vram_update(NULL);
    if (!skip_to_line) {
      scroll(render_to_nt_b ? 0x100 : 0, 0);
      render_to_nt_b = !render_to_nt_b;
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
    render();
    gray_line();
  }
}

char vram_buf_idx;
char vram_buf[40];
char cur_line;

void draw_vert_line(char color, char x, char y_top, char y_bot) {
  if (unfinished)
    return;
  if (cur_line < skip_to_line) {
    cur_line++;
    return;
  }
  if (vram_buf_idx + y_bot - y_top + 4 >= sizeof(vram_buf)) {
    skip_to_line = cur_line;
    unfinished = true;
    return;
  }
  cur_line++;
  unsigned vram_addr = render_to_nt_b ? NTADR_B(x, y_top) : NTADR_A(x, y_top);
  vram_buf[vram_buf_idx++] = vram_addr >> 8 | NT_UPD_VERT;
  vram_buf[vram_buf_idx++] = vram_addr & 0xff;
  vram_buf[vram_buf_idx++] = y_bot - y_top + 1;
  for (char y = y_top; y <= y_bot; ++y)
    vram_buf[vram_buf_idx++] = color;
}

void render() {
  unfinished = false;
  vram_buf_idx = 0;
  cur_line = 0;
  for (int x = 0; x < 32; x++)
    draw_vert_line(0, x, 0, 29);
  if (y_top <= y_bot)
    draw_vert_line(1, x, y_top, y_bot);
  vram_buf[vram_buf_idx] = NT_UPD_EOF;
  set_vram_update(vram_buf);
  if (!unfinished)
    skip_to_line = 0;
}
