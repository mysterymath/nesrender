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

.section .text.render_span_left,"ax",@progbits
.balign 256
render_span_left:
  .rept FRAMEBUFFER_HEIGHT
    sta framebuffer_columns,y
    iny
  .endr
  jmp .Lrender_span_left_return

.section .text.render_span_right,"ax",@progbits
.balign 256
render_span_right:
  .rept FRAMEBUFFER_HEIGHT
    sta framebuffer_columns+FRAMEBUFFER_HEIGHT,y
    iny
  .endr
  jmp .Lrender_span_right_return

.rodata
render_span_offsets:
  i = 0
  .rept FRAMEBUFFER_HEIGHT+1
    .byte (FRAMEBUFFER_HEIGHT - i) * (3 + 1) ; sta absidx, iny
    i = i + 1
  .endr

.text
.globl render_span_buffer_left
render_span_buffer_left:
  lda #>render_span_left
  sta __rc5

  lda span_buffer ; Size
  sta __rc2
  ldx #0
  ldy #0
.Lloop_left:
  lda span_buffer+1+FRAMEBUFFER_HEIGHT,x ; Length
  stx __rc3
  tax
  lda render_span_offsets,x
  sta __rc4
  ldx __rc3

  ; Render the span
  lda span_buffer+1,x ; Color
  jmp (__rc4)

.Lrender_span_left_return:
  inx
  cpx __rc2
  bne .Lloop_left
  rts

.text
.globl render_span_buffer_right
render_span_buffer_right:
  lda #>render_span_right
  sta __rc5

  lda span_buffer ; Size
  sta __rc2
  ldx #0
  ldy #0
.Lloop_right:
  lda span_buffer+1+FRAMEBUFFER_HEIGHT,x ; Length
  stx __rc3
  tax
  lda render_span_offsets,x
  sta __rc4
  ldx __rc3

  ; Render the span
  lda span_buffer+1,x ; Color
  jmp (__rc4)

.Lrender_span_right_return:
  inx
  cpx __rc2
  bne .Lloop_right
  rts
