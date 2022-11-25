#include <nesdoug.h>
#include <neslib.h>

#include <string.h>

// Framebuffers for the current and next frame, with 2 bits per pixel, in
// column major order. Each pixel corresponds to one of the tiles in the NES
// nametable and controls its color.
char fb_cur[240];
char fb_next[240];

void render();
void present();

char x = 1;
char y_top = 2;
char y_bot = 3;
bool control_top;

int main() {
  static const char bg_pal[16] = {0x00, 0x11, 0x16, 0x1a};
  ppu_off();
  pal_bg_bright(4);
  pal_bg(bg_pal);
  ppu_on_bg();

  while (true) {
    ppu_wait_nmi();
    set_vram_update(NULL);
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
    render();
    present();
    gray_line();
  }
}

void draw_vert_line(char color, char x, char y_top, char y_bot) {
  char offset = (x * 30 + y_top) / 4;
  char shift = (x * 30 + y_top) % 4;
  char y = y_top - shift;
  // Draw portion of line before we get to the full-byte region.
  if (shift) {
    char and_mask = 0;
    char or_mask = 0;
    for (char s = 0; s < 4; ++s, ++y) {
      and_mask >>= 2;
      or_mask >>= 2;
      if (s < shift || y > y_bot)
        and_mask |= 0b11000000;
      else
        or_mask |= color << 6;
    }
    fb_next[offset] &= and_mask;
    fb_next[offset] |= or_mask;
  }
}

void render() {
  memset(fb_next, 0, sizeof(fb_next));
  draw_vert_line(1, 3, 4, 5);
}

constexpr char max_updates_per_frame = 32;
char vram_buf[max_updates_per_frame * 3 + 1];

__attribute__((noinline)) void present() {
  char x = 0, y = 0, update_idx = 0, updates_left = max_updates_per_frame;
  for (char i = 0; i < sizeof(fb_next) && updates_left; ++i) {
    char cur = fb_cur[i];
    char next = fb_next[i];

    if (next == cur) {
      y += 4;
      if (y >= 30) {
        y -= 30;
        ++x;
      }
      continue;
    }

    for (char j = 0; j < 4; ++j) {
      char cur_lo = cur & 0b11;
      char next_lo = next & 0b11;

      if (next_lo != cur_lo) {
        if (!updates_left--)
          goto done;
        unsigned addr = NTADR_A(x, y);
        vram_buf[update_idx++] = addr >> 8;
        vram_buf[update_idx++] = addr & 0xff;
        vram_buf[update_idx++] = next_lo;
      }

      cur >>= 2;
      next >>= 2;
      if (++y == 30) {
        y = 0;
        ++x;
      }
    }
    fb_cur[i] = fb_next[i];
  }
done:
  if (updates_left != max_updates_per_frame) {
    vram_buf[update_idx] = NT_UPD_EOF;
    set_vram_update(vram_buf);
  }
}
