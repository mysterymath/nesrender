.include "imag.inc"

.globl draw_solid_column_even
draw_solid_column_even:
	cpx __rc4
	bne 1f
	rts
1:
	sta __rc5
	asl
	asl
	sta __rc6

	lda __rc4
	lsr
	sta __rc7

	txa
	lsr
	tay
	bcc 1f

	lda (__rc2),y
	and #0b11110011
	ora __rc6
	sta (__rc2),y
	iny
1:
	cpy __rc7
	beq 2f
	lda __rc6
	ora __rc5
	sta __rc6
.Leven_loop:
	lda (__rc2),y
	and #0b11110000
	ora __rc6
	sta (__rc2),y
	iny
	cpy __rc7
	bne .Leven_loop
2:
	lda __rc4
	and #1
	bne 1f
	rts
1:
	lda (__rc2),y
	and #0b11111100
	ora __rc5
	sta (__rc2),y
	rts

.globl draw_solid_column_odd
draw_solid_column_odd:
	cpx __rc4
	bne 1f
	rts
1:
	sta __rc5
	lda #0
	lsr __rc5
	ror
	lsr __rc5
	ror
	sta __rc5
	lsr
	lsr
	sta __rc6

	lda __rc4
	lsr
	sta __rc7

	txa
	lsr
	tay
	bcc 1f

	lda (__rc2),y
	and #0b00111111
	ora __rc5
	sta (__rc2),y
	iny
1:
	cpy __rc7
	beq 2f
	lda __rc5
	ora __rc6
	sta __rc5
.Lodd_loop:
	lda (__rc2),y
	and #0b00001111
	ora __rc5
	sta (__rc2),y
	iny
	cpy __rc7
	bne .Lodd_loop
2:
	lda __rc4
	and #1
	bne 1f
	rts
1:
	lda (__rc2),y
	and #0b11001111
	ora __rc6
	sta (__rc2),y
	rts
