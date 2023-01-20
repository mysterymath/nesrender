.globl lin_to_log
lin_to_log:
	cpx #0
	bne 1f
	cmp #0
	bne 1f
	lda #0
	ldx #$80
	rts

1:
	cpx #$80
	bne 1f
	cmp #0
	bne 1f
	lda #$ff
	ldx #$7f
	rts

1:
	cpx #0
	bpl 1f
	sta __rc2
	stx __rc3
	lda #0
	sec
	sbc __rc2
	tay
	lda #0
	sbc __rc3
	tax
	tya

1:
	cpx #1
	bne 1f
	tay
	lda logt_hi,y
	tax
	lda logt_lo,y
	rts

1:
	sta __rc2
	stx __rc3
	lda #0
	cpx #0
	bne .Lshift_right

.Lshift_left:
	asl __rc2
	rol __rc3
	clc
	adc #248
	ldx __rc3
	cpx #1
	bne .Lshift_left
	ldy __rc2
	clc
	adc logt_hi,y
	tax
	lda logt_lo,y
	rts

.Lshift_right:
	lsr __rc3
	ror __rc2
	clc
	adc #8
	ldx __rc3
	cpx #1
	bne .Lshift_right

	ldy __rc2
	clc
	adc logt_hi,y
	tax
	lda logt_lo,y
	rts

;.globl log_to_lin
;log_to_lin:
;	sta __rc2
;	txa
;	lsr
;	ror __rc2
;	lsr
;	ror __rc2
;	lsr
;	ror __rc2
;	sta __rc3
;	ldx __rc2
;
;	lda #15
;	sec
;	sbc __rc3
;	tay
;	cmp #8
;	bmi .Lshift_lt_8
;	sec
;	sbc #8
;	tay
;	lda #0
;	sta __rc2
;	lda alogt_hi,x
;	cpy #0
;	beq .Ldone
;	jmp .Lshift_loop
;
;.Lshift_lt_8:
;	lda alogt_hi,y
;	sta __rc2
;	lda alogt_lo,y
;	cpy #0
;	beq .Ldone
;
;.Lshift_loop:
;	lsr __rc2
;	ror
;	dey
;	bne .Lshift_loop
;
;.Ldone:
;	ldx __rc2
;	rts
;
