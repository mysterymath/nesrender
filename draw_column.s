.include "imag.inc"

; void draw_column_even(uint8_t color, uint8_t *col, uint8_t y_top,
;                       uint8_t y_bot) {
;   uint8_t i = y_top / 2;
;   if (y_top & 1) {
;     col[i] &= 0b11110011;
;     col[i] |= color << 2;
;     i++;
;   }
;   while (i < y_bot / 2) {
;     col[i] &= 0b11110000;
;     col[i] |= color << 2 | color << 0;
;     i++;
;   }
;   if (y_bot & 1) {
;     col[i] &= 0b11111100;
;     col[i] |= color << 0;
;   }
; }

.globl draw_column_even
draw_column_even:
  sta __rc5
	asl
	asl
	sta __rc6
	ora __rc5
	sta __rc7

	lsr __rc4
	lda #0
	ror
	sta __rc8

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
	cpy __rc4
	beq 1f

2:
	lda (__rc2),y
	and #0b11110000
	ora __rc7
	sta (__rc2),y
	iny

	cpy __rc4
	bne 2b

1:
	lda __rc8
	beq 1f

	lda (__rc2),y
	and #0b11111100
	ora __rc5
	sta (__rc2),y

1:
	rts
