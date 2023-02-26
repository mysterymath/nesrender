.include "imag.inc"
.zeropage present_x, present_next_col, present_cur_col

.Lpresent_vram_buf = __rc2
.Lpresent_y = __rc4
.Lpresent_vram = __rc5
.zeropage .Lpresent_vram_buf, .Lpresent_y, .Lpresent_vram

.text
.globl present
present:
  lda still_presenting
  bne 1f
  lda #0
  sta present_x
  lda #<fb_next
  sta present_next_col
  lda #>fb_next
  sta present_next_col+1
  lda #<fb_cur
  sta present_cur_col
  lda #>fb_cur
  sta present_cur_col+1
1:
  lda #<vram_buf
  sta .Lpresent_vram_buf
  lda #>vram_buf
  sta .Lpresent_vram_buf+1
  ldy #29
.Lloop:
  lda (present_next_col),y
  cmp (present_cur_col),y
  bne 1f
  dey
  bne .Lloop
  jmp .Lnext_x
1:
  tax
  lda .Lpresent_vram_buf+1
  cmp #>(vram_buf + 1024)
  bne 1f
  lda .Lpresent_vram_buf
  cmp #<(vram_buf + 1024)
  bne 1f
  lda #1
  sta still_presenting
  bne .Ldone
1:
  sty .Lpresent_y
  ldy #0
  lda #0xea
  sta (.Lpresent_vram_buf),y
  sty .Lpresent_vram
  lda .Lpresent_y
  ; vram = 0x2000 + x + y * 32 == x + y << 5 == x + (y << 8) >> 3
  lsr
  ror .Lpresent_vram
  lsr
  ror .Lpresent_vram
  lsr
  ror .Lpresent_vram
  sta .Lpresent_vram+1
  clc
  lda .Lpresent_vram
  adc present_x
  sta .Lpresent_vram
  lda .Lpresent_vram+1
  adc #$20 ; >NAMETABLE_A
  sta .Lpresent_vram+1
  lda .Lpresent_vram+1
  ldy #2
  sta (.Lpresent_vram_buf),y
  lda .Lpresent_vram
  ldy #7
  sta (.Lpresent_vram_buf),y
  txa
  ldy #12
  sta (.Lpresent_vram_buf),y
  ldy .Lpresent_y
  sta (present_cur_col),y
  clc
  lda .Lpresent_vram_buf
  adc #16
  sta .Lpresent_vram_buf
  bcc 1f
  inc .Lpresent_vram_buf+1
1:
  dey
  bne .Lloop
.Lnext_x:
  clc
  lda present_cur_col
  adc #30
  sta present_cur_col
  bcc 1f
  inc present_cur_col+1
1:
  clc
  lda present_next_col
  adc #30
  sta present_next_col
  bcc 1f
  inc present_next_col+1
1:
  ldy #29
  inc present_x
  lda present_x
  cmp #32
  beq 1f
  jmp .Lloop
1:
  lda #0
  sta still_presenting
.Ldone:
  ldy #0
  lda #$60
  sta (.Lpresent_vram_buf),y
  iny
  sty updating_vram
  rts
