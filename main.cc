#include <neslib.h>

// Framebuffers for the previous and next frame, with 2 bits per pixel. Each
// pixel corresponds to one of the tiles in the NES nametable and controls its
// color.
char fb_prev[240];
char fb_next[240];

void random_diffs();
void render();

int main() {
  static const char bg_pal[16] = {0x00, 0x11, 0x16, 0x1a};
  ppu_off();
  pal_bg_bright(4);
  pal_bg(bg_pal);
  ppu_on_bg();

  while (true) {
    ppu_wait_nmi();
    char pad_t = pad_trigger(0);
    if (pad_t & PAD_A)
      random_diffs();
    render();
  }
}

// Later, produce a random smattering of differences. For now, just cycle the
// color of the top left pixel.
void random_diffs() {
  char cur = (fb_next[0] + 1) & 0b11;
  fb_next[0] &= 0b11111100;
  fb_next[0] |= cur;
}

void render() {
  constexpr int kMaxUpdatesPerFrame;
}
