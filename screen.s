
.include "nes.inc"
.include "imag.inc"
.zeropage present_x, present_next_col, present_cur_col

.Lpresent_vram_buf = __rc2 ; +1
.Lpresent_y = __rc4
.Lpresent_vram = __rc5 ; +1
.Lpresent_cycles_remaining = __rc7 ; +1
.zeropage .Lpresent_vram_buf, .Lpresent_y, .Lpresent_vram, .Lpresent_cycles_remaining

.Lmax_cycles = 1024 + 512 + 128 + 8 ; Empirically determined by not checking against fb_cur

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

	lda #<.Lmax_cycles
	sta .Lpresent_cycles_remaining
	lda #>.Lmax_cycles
	sta .Lpresent_cycles_remaining+1

	ldy #29
.Lloop:
	lda (present_next_col),y
	cmp (present_cur_col),y
	bne 1f
	dey
	bpl .Lloop
	jmp .Lnext_x
1:
	tax

	sec
	lda .Lpresent_cycles_remaining
	sbc #(2 + 4 + 2 + 4 + 2 + 4)
	sta .Lpresent_cycles_remaining
	lda .Lpresent_cycles_remaining+1
	sbc #0
	sta .Lpresent_cycles_remaining+1
	bcs 1f

	lda #1
	sta still_presenting
	beq 1f
	jmp .Ldone
1:
	sty .Lpresent_y
	tya
	ldy #0
	sty .Lpresent_vram
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

	lda #$a9 ; lda #
	sta (.Lpresent_vram_buf),y
	iny
	lda .Lpresent_vram+1
	sta (.Lpresent_vram_buf),y
	iny

	lda #$8d ; sta abs
	sta (.Lpresent_vram_buf),y
	iny
	lda #<PPUADDR
	sta (.Lpresent_vram_buf),y
	iny
	lda #>PPUADDR
	sta (.Lpresent_vram_buf),y
	iny

	lda #$a9 ; lda #
	sta (.Lpresent_vram_buf),y
	iny
	lda .Lpresent_vram
	sta (.Lpresent_vram_buf),y
	iny

	lda #$8d ; sta abs
	sta (.Lpresent_vram_buf),y
	iny
	lda #<PPUADDR
	sta (.Lpresent_vram_buf),y
	iny
	lda #>PPUADDR
	sta (.Lpresent_vram_buf),y
	iny

	lda #$a9 ; lda #
	sta (.Lpresent_vram_buf),y
	iny
	txa	; color
	sta (.Lpresent_vram_buf),y
	iny

	lda #$8d ; sta abs
	sta (.Lpresent_vram_buf),y
	iny
	lda #<PPUDATA
	sta (.Lpresent_vram_buf),y
	iny
	lda #>PPUDATA
	sta (.Lpresent_vram_buf),y
	iny

	ldy .Lpresent_y
	txa
	sta (present_cur_col),y
	clc
	lda .Lpresent_vram_buf
	adc #15
	sta .Lpresent_vram_buf
	bcc 1f
	inc .Lpresent_vram_buf+1
1:
	dey
	bmi .Lnext_x
	jmp .Lloop
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
