
.include "nes.inc"
.include "imag.inc"
.zeropage present_x, present_next_col, present_cur_col

.Lvram_buf = __rc2 ; +1
.Ly = __rc4
.Lvram = __rc5 ; +1
.Lcycles_remaining = __rc7 ; +1
.Lis_consec = __rc9
.zeropage .Lvram_buf, .Ly, .Lvram, .Lcycles_remaining, .Lis_consec

.Lmax_cycles = 1024 + 512 + 128 + 2 ; Empirically determined by not checking against fb_cur

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
	sta .Lvram_buf
	lda #>vram_buf
	sta .Lvram_buf+1

	lda #<.Lmax_cycles
	sta .Lcycles_remaining
	lda #>.Lmax_cycles
	sta .Lcycles_remaining+1

	lda #0
	sta .Lis_consec

	ldy #0
.Lloop:
	lda (present_next_col),y
	cmp (present_cur_col),y
	bne .Lsend_pixel
	iny
	cpy #30
	bne .Lloop
.Lnext_x:
	lda #0
	sta .Lis_consec

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
	ldy #0
	inc present_x
	lda present_x
	cmp #32
	bne .Lloop
1:
	lda #0
	sta still_presenting
.Ldone:
	ldy #0
	lda #$60
	sta (.Lvram_buf),y
	iny
	sty updating_vram
	rts
.Lsend_pixel:
	; x := color
	tax

	sty .Ly

	lda .Lis_consec
	beq .Lnon_consec
	ldy #0
	jmp .Lconsec
.Lnon_consec:
	; Decrement cycles remaining
	sec
	lda .Lcycles_remaining
	sbc #(2 + 4 + 2 + 4)
	sta .Lcycles_remaining
	lda .Lcycles_remaining+1
	sbc #0
	sta .Lcycles_remaining+1
	bcs 1f

	; Quit if too few.
	lda #1
	sta still_presenting
	jmp .Ldone
1:

	; vram = y * 32
	tya
	ldy #0
	sty .Lvram
	lsr
	ror .Lvram
	lsr
	ror .Lvram
	lsr
	ror .Lvram
	sta .Lvram+1

  ; vram += NAMETABLE_A + x
	clc
	lda .Lvram
	adc present_x
	sta .Lvram
	lda .Lvram+1
	adc #$20 ; >NAMETABLE_A
	sta .Lvram+1

	lda #$a9 ; lda #
	sta (.Lvram_buf),y
	iny
	lda .Lvram+1
	sta (.Lvram_buf),y
	iny

	lda #$8d ; sta abs
	sta (.Lvram_buf),y
	iny
	lda #<PPUADDR
	sta (.Lvram_buf),y
	iny
	lda #>PPUADDR
	sta (.Lvram_buf),y
	iny

	lda #$a9 ; lda #
	sta (.Lvram_buf),y
	iny
	lda .Lvram
	sta (.Lvram_buf),y
	iny

	lda #$8d ; sta abs
	sta (.Lvram_buf),y
	iny
	lda #<PPUADDR
	sta (.Lvram_buf),y
	iny
	lda #>PPUADDR
	sta (.Lvram_buf),y
	iny

	lda #1
	sta .Lis_consec

.Lconsec:
	; Decrement cycles remaining
	sec
	lda .Lcycles_remaining
	sbc #(2 + 4)
	sta .Lcycles_remaining
	lda .Lcycles_remaining+1
	sbc #0
	sta .Lcycles_remaining+1
	bcs 1f

	; Ran out of cycles; quit.
	lda #1
	sta still_presenting
	jmp .Ldone
1:

	lda #$a9 ; lda #
	sta (.Lvram_buf),y
	iny
	txa	; color
	sta (.Lvram_buf),y
	iny

	lda #$8d ; sta abs
	sta (.Lvram_buf),y
	iny
	lda #<PPUDATA
	sta (.Lvram_buf),y
	iny
	lda #>PPUDATA
	sta (.Lvram_buf),y
	iny

	clc
	tya
	adc .Lvram_buf
	sta .Lvram_buf
	bcc 1f
	inc .Lvram_buf+1
1:

	ldy .Ly
	txa
	sta (present_cur_col),y

	iny
	cpy #30
	beq 1f
	lda (present_next_col),y
	cmp (present_cur_col),y
	beq 2f
	; x := color
	tax
	sty .Ly
	ldy #0
	jmp .Lconsec
2:
	ldx #0
	stx .Lis_consec
	iny
	cpy #30
	beq 1f
	jmp .Lloop
1:
	jmp .Lnext_x
