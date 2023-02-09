.include "imag.inc"

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

.globl log_to_lin
log_to_lin:
	cpx #0
	bpl .Lexp_nonnegative

	; Note: Signed CMP with same sign cannot overflow (-1/2)
	cpx #$f8
	bmi .Labs_zero
	bne .Labs_one
	cmp #0
	bne .Labs_one
.Labs_zero:
	lda #0
	tax
	rts
.Lexp_nonnegative:
	bne .Lexp_positive
	cmp #0
	bne .Lexp_positive
.Labs_one:
	lda __rc2
	bne 1f
	lda #1
	ldx #0
	rts
1:
	lda #$ff
	ldx #$ff
	rts

.Lexp_positive:
	; (2^15)
	cpx #$78
	bmi .Lexp_in_range
	lda __rc2
	bne 1f
	lda #$ff
	ldx #$7f
	rts
1:
	lda #0
	ldx #$80
	rts

.Lexp_in_range:
	sta __rc3
	txa
	lsr
	ror __rc3
	lsr
	ror __rc3
	lsr
	ror __rc3

	sta __rc4
	ldx __rc3
	lda alogt_lo,x
	sta __rc5
	lda alogt_hi,x
	sta __rc6

	lda #15
	sec
	sbc __rc4
	sbc #8
	bcc .Lshift_lt_8
	tax
	lda __rc6
	cpx #0
	beq .Lshift_8_done
.Lshift_8:
	lsr
	dex
	bne .Lshift_8
.Lshift_8_done:
	stx __rc6
	beq .Ldone
.Lshift_lt_8:
	adc #8
	tax
	lda __rc5
.Lshift_16:
	lsr __rc6
	ror
	dex
	bne .Lshift_16
.Ldone:
	ldy __rc2
	bne 1f
	ldx __rc6
	rts
1:
	clc
	eor #$ff
	adc #1
	tay
	lda __rc6
	eor #$ff
	adc #0
	tax
	tya
	rts
