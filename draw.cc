#include "draw.h"

#include "screen.h"

__attribute__((no_builtin("memset"))) void clear_screen() {
  for (int i = 0; i < 256; i++) {
    fb_next[i] = 0;
    (fb_next + 256)[i] = 0;
    (fb_next + 512)[i] = 0;
  }
  for (uint8_t i = 0; i < 192; i++)
    (fb_next + 768)[i] = 0;
}
