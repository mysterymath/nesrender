#include <nesdoug.h>
#include <neslib.h>

#include <string.h>

// Framebuffers for the current and next frame, with 2 bits per pixel, in
// column major order. Each pixel corresponds to one of the tiles in the NES
// nametable and controls its color.
char fb_cur[240];
char fb_next[240];

void mutate();
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
      mutate();
    present();
    gray_line();
  }
}

void mutate() {
  char color = (fb_next[0] + 1) & 0b11;
  memset(fb_next, color | color << 2 | color << 4 | color << 6,
         sizeof(fb_next));
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
