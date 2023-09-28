#include "framebuffer-constants.h"

; The framebuffer is maintained as a mutable routine to copy the contents to the
; PPU as quickly as possible.
.section .prg_ram_0,"aw",@nobits
.globl framebuffer
framebuffer:
  .fill 24*32*(2+3)+1 ; 24 tiles x (LDA imm + STA abs) + RTS

.globl render_framebuffer_columns

.text
render_framebuffer_columns:
  tay

  i = 0
  .rept FRAMEBUFFER_HEIGHT_TILES
    lda framebuffer_columns + FRAMEBUFFER_HEIGHT + (i * 2) + 1
    asl
    asl
    ora framebuffer_columns + FRAMEBUFFER_HEIGHT + (i * 2)
    asl
    asl
    ora framebuffer_columns + (i * 2) + 1
    asl
    asl
    ora framebuffer_columns + (i * 2)
    sta (framebuffer_row_ptrs + i * 2),y
    i = i + 1
  .endr

  rts