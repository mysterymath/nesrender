#include <neslib.h>

int main() {
  static const char bg_pal[16] = {0x00, 0x11, 0x16, 0x1a};
  ppu_off();
  pal_bg_bright(4);
  pal_bg(bg_pal);
  ppu_on_bg();
}
