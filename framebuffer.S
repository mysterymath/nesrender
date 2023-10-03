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

.section .text.render_span_left_core,"ax",@progbits
.balign 256
render_span_left_core:
  .rept FRAMEBUFFER_HEIGHT
    sta framebuffer_columns,y
    iny
  .endr
  tya
  rts

.section .text.render_span_right_core,"ax",@progbits
.balign 256
render_span_right_core:
  .rept FRAMEBUFFER_HEIGHT
    sta framebuffer_columns+FRAMEBUFFER_HEIGHT,y
    iny
  .endr
  tya
  rts

.rodata
render_span_offsets:
  i = 0
  .rept FRAMEBUFFER_HEIGHT+1
    .byte (FRAMEBUFFER_HEIGHT - i) * (3 + 1) ; sta absidx, iny
    i = i + 1
  .endr

.text
.globl render_span_left
render_span_left:
  tay ; Position
  lda #>render_span_left_core
  sta __rc5
  lda render_span_offsets,x  ; length -> offset
  sta __rc4
  lda __rc2 ; Color
  jmp (__rc4)

.text
.globl render_span_right
render_span_right:
  tay ; Position
  lda #>render_span_right_core
  sta __rc5
  lda render_span_offsets,x  ; length -> offset
  sta __rc4
  lda __rc2 ; Color
  jmp (__rc4)
