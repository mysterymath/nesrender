#include <nesdoug.h>
#include <neslib.h>

// Framebuffers for the previous and next frame, with 2 bits per pixel, in
// column major order. Each pixel corresponds to one of the tiles in the NES
// nametable and controls its color.
char fb_prev[240];
char fb_next[240];

void random_diffs();
void present();

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
    if (pad_t & PAD_A)
      random_diffs();
    present();
    gray_line();
  }
}

void random_diffs() {
  constexpr char num_updates = 240;
  for (char i = 0; i < num_updates; ++i) {
    fb_next[i] = rand8();
  }
}

constexpr char max_updates_per_frame = 4;
char vram_buf[max_updates_per_frame * 3 + 1];

__attribute__((noinline)) void present() {
  char x = 0, y = 0, update_idx = 0, updates_left = max_updates_per_frame;
  bool any_updates = false;
  for (char i = 0; i < sizeof(fb_next); ++i) {
    char prev = fb_prev[i];
    char next = fb_next[i];

    if (next == prev) {
      y += 4;
      if (y > 32) {
        y -= 32;
        ++x;
      }
      continue;
    }

    for (char j = 0; j < 4; ++j) {
      char prev_lo = prev & 0b11;
      char next_lo = next & 0b11;

      if (next_lo != prev_lo) {
        any_updates = true;
        unsigned addr = NTADR_A(x, y);
        vram_buf[update_idx++] = addr >> 8;
        vram_buf[update_idx++] = addr & 0xff;
        vram_buf[update_idx++] = next_lo;
        if (!--updates_left)
          goto done;
      }

      prev <<= 2;
      next <<= 2;
      if (++y == 33) {
        y = 0;
        ++x;
      }
    }
    fb_prev[i] = fb_next[i];
  }
done:
  if (any_updates) {
    vram_buf[update_idx] = NT_UPD_EOF;
    set_vram_update(vram_buf);
  }
}
