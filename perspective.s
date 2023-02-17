; TODO: Fix memset lowering
.globl clear_coverage
clear_coverage:
	ldx #60	; screen_height
	ldy #63
1:
	lda #0
	sta coverage_py_tops,y
	txa
	sta coverage_py_bots,y
	dey
	bne 1b

	lda #0
	sta coverage_py_tops,y
	txa
	sta coverage_py_bots,y
	rts

; TODO: Fix memset lowering
.globl clear_col_z
clear_col_z:
	lda #$ff
	ldy #63
1:
	sta col_z_lo,y
	sta col_z_hi,y
	dey
	bne 1b

	sta col_z_lo,y
	sta col_z_hi,y
	rts
