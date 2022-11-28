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
  scroll(0x100, 0);
  ppu_on_bg();

  while (true) {
    ppu_wait_nmi();
    set_vram_update(NULL);
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

void present() {
  char vram_buf_idx = 0;
  static char vram_buf[80];
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

  unsigned offset = 0;
  still_presenting = false;
  for (char x = 0; x < 32; x++) {
    for (char y = 0; y < 30; y++) {
      char chain = 0xff, chain_len_idx = 0;
      if (fb_next[offset] == chain) {
        if (vram_buf_idx + 1 >= sizeof(vram_buf)) {
          still_presenting = true;
          goto done;
        }
        vram_buf[vram_buf_idx++] = fb_next[offset];
        vram_buf[chain_len_idx]++;
        if (present_to_nt_b)
          fb_b[offset] = fb_next[offset];
        else
          fb_a[offset] = fb_next[offset];
        ++offset;
        continue;
      }
      if (vram_buf_idx + 4 >= sizeof(vram_buf)) {
        still_presenting = true;
        goto done;
      }
      if (present_to_nt_b) {
        if (fb_next[offset] != fb_b[offset]) {
          unsigned addr = NTADR_B(x, y);
          vram_buf[vram_buf_idx++] = NT_UPD_VERT | addr >> 8;
          vram_buf[vram_buf_idx++] = addr & 0xff;
          chain_len_idx = vram_buf_idx++;
          vram_buf[chain_len_idx] = 1;
          vram_buf[vram_buf_idx++] = fb_next[offset];
          fb_b[offset] = fb_next[offset];
          chain = fb_next[offset];
        }
      } else {
        if (fb_next[offset] != fb_a[offset]) {
          unsigned addr = NTADR_A(x, y);
          vram_buf[vram_buf_idx++] = NT_UPD_VERT | addr >> 8;
          vram_buf[vram_buf_idx++] = addr & 0xff;
          chain_len_idx = vram_buf_idx++;
          vram_buf[chain_len_idx] = 1;
          vram_buf[vram_buf_idx++] = fb_next[offset];
          fb_a[offset] = fb_next[offset];
          chain = fb_next[offset];
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
