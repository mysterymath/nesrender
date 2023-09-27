#include <ines.h>
#include <nes.h>
#include <peekpoke.h>
#include <soa.h>
#include <stdint.h>

// Configure for SNROM MMC1 board.
MAPPER_CHR_ROM_KB(8);
MAPPER_PRG_RAM_KB(8);

typedef uint16_t u16;
typedef uint8_t u8;

__attribute__((section(".zp.bss"))) volatile u8 frame_count;

/* 16-bit xorshift PRNG */
u16 xorshift() {
  static u16 x = 1;
  x ^= x << 7;
  x ^= x >> 9;
  x ^= x << 8;
  return x;
}

constexpr u16 ppu_bg_pals = 0x3f00;
constexpr u16 ppu_spr_pals = 0x3f11;

struct OAM {
  u8 y;
  u8 tile;
  u8 attrs;
  u8 x;
};

alignas(256) OAM oam_buf[64];

constexpr u8 fb_height_tiles = 21;
constexpr u8 fb_width_tiles = 32;

constexpr u8 fb_height = fb_height_tiles * 2;
constexpr u8 fb_width = fb_width_tiles * 2;

__attribute__((always_inline)) static inline void mmc1_register_write(u16 addr,
                                                                      u8 val) {
  POKE(addr, val);
  val >>= 1;
  POKE(addr, val);
  val >>= 1;
  POKE(addr, val);
  val >>= 1;
  POKE(addr, val);
  val >>= 1;
  POKE(addr, val);
}

static void ppu_set_addr(u16 addr) {
  (void)PPU.status;
  PPU.vram.address = addr >> 8;
  PPU.vram.address = addr & 0xff;
}

volatile uint8_t c;

constexpr u16 mmc1_ctrl = 0x8000;

constexpr u8 frame_buffer_stride = 2 + 3; // LDA imm, STA abs
extern volatile u8
    frame_buffer[frame_buffer_stride * fb_height_tiles * fb_width_tiles + 1];

// Two columns of the frame buffer, expanded out to one byte per pixel.
struct FrameBufferColumns {
  u8 pixels[2][fb_height];

  void randomize();

  void render(u8 tile_x) const;
};

FrameBufferColumns frame_buffer_columns;

[[clang::noinline]] void FrameBufferColumns::randomize() {
  for (u8 x = 0; x < 2; x++)
    for (u8 y = 0; y < fb_height; y++)
      pixels[x][y] = xorshift() % 4;
}

[[clang::noinline]] void FrameBufferColumns::render(u8 tile_x) const {
  volatile u8 *fb = frame_buffer + 1 + tile_x * frame_buffer_stride;
#pragma clang loop unroll(full)
  for (u8 y = 0; y < fb_height; y += 2) {
    u8 color = pixels[1][y + 1];
    color <<= 2;
    color |= pixels[1][y];
    color <<= 2;
    color |= pixels[0][y + 1];
    color <<= 2;
    color |= pixels[0][y];
    *fb = color;
    fb += frame_buffer_stride * fb_width_tiles;
  }
}

// The first row is corrupted, so black it out.
static void init_first_row() {
  ppu_set_addr(0x2000);
  for (u8 i = 0; i < fb_width_tiles; i++)
    PPU.vram.data = 0;
}

static void init_framebuffer() {
  u16 tile = 0;
  for (u16 i = 0; i < sizeof(frame_buffer);) {
    static constexpr u8 lda_imm = 0xa9;
    frame_buffer[i++] = lda_imm;
    frame_buffer[i++] = tile++;

    static constexpr u8 sta_abs = 0x8d;
    frame_buffer[i++] = sta_abs;
    frame_buffer[i++] = reinterpret_cast<u16>(&PPU.vram.data) & 0xff;
    frame_buffer[i++] = reinterpret_cast<u16>(&PPU.vram.data) >> 8;
  }
  static constexpr u8 rts = 0x60;
  frame_buffer[sizeof(frame_buffer) - 1] = rts; // RTS
}

// Fill the hud w/ a nice checkerboard pattern and zero the attribute table.
static void init_nametable_remainder() {
  ppu_set_addr(0x2000 + fb_height_tiles * fb_width_tiles);
  u16 i;
  for (i = fb_height_tiles * fb_width_tiles; i < 30 * 32; i++)
    PPU.vram.data = 86;
  // Zero the attribute table.
  for (; i < 1024; i++)
    PPU.vram.data = 0;
}

void update() {
  // TODO
}

int main() {
  init_first_row();
  init_framebuffer();
  init_nametable_remainder();

  mmc1_register_write(mmc1_ctrl, 0b01100);
  PPU.control = 0b00001000;

  ppu_set_addr(ppu_bg_pals);
  static const uint8_t bg_pals[16] = {0x0f, 0x06, 0x16, 0x0c};
  for (u16 i = 0; i < sizeof(bg_pals); i++)
    PPU.vram.data = bg_pals[i];

  ppu_set_addr(ppu_spr_pals);
  static const uint8_t spr_pals[15] = {0x00, 0x10, 0x30};
  for (u16 i = 0; i < sizeof(spr_pals); i++)
    PPU.vram.data = spr_pals[i];

  // FPS counter
  oam_buf[0].x = 8;
  oam_buf[0].y = 24;
  oam_buf[1].x = 16;
  oam_buf[1].y = 24;

  PPU.control = 0b10001000;
  PPU.mask = 0b0011110;

  // TODO: This needs work
  uint8_t last_update = frame_count;
  while (true) {
    uint8_t cur_update = frame_count;
    uint8_t fps =
        60 / (cur_update < last_update ? cur_update + 256 - last_update
                                       : cur_update - last_update);

    // Handle wraparound if any.
    if (cur_update < last_update) {
      while (!__builtin_add_overflow(last_update, 3, &last_update))
        ;
    }
    for (; last_update < cur_update; last_update += 3)
      update();
    oam_buf[0].tile = fps / 10;
    oam_buf[1].tile = fps % 10;

    for (u8 x_tile = 0; x_tile < fb_width_tiles; ++x_tile)
      frame_buffer_columns.render(x_tile);
  }
}
