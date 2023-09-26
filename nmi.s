.include "nes.inc"

; The framebuffer is maintained as a mutable routine to copy the contents to the
; PPU as quickly as possible.
.section .prg_ram_0,"aw",@nobits
.globl frame_buffer
frame_buffer:
  .fill 24*32*(2+3)+1 ; 24 tiles x (LDA imm + STA abs) + RTS

.section .nmi.100,"axR",@progbits
  lda PPUSTATUS

  ; Disable rendering; we need slightly more than one vblank.
  lda #0
  sta PPUMASK

  ; Run the framebuffer copy routine.
  lda #>$2000
  sta PPUADDR
  lda #<$2000
  sta PPUADDR
  jsr frame_buffer

  ; The scroll is still zero, but we've skipped the point where it would be
  ; copied over to PPUADDR. Do this manually.
  lda #0
  sta PPUSCROLL
  sta PPUSCROLL
  lda #>$2000
  sta PPUADDR
  lda #<$2000
  sta PPUADDR

  ; Run OAMDMA last, since its previous value may have decayed
  lda #0
  sta OAMADDR
  lda #>oam_buf
  sta OAMDMA

  ; Empirically add NOPs until it doesn't glitch 
  .rept 0
  nop
  .endr

  ; Re-enable rendering.
  lda #0b00011110
  sta PPUMASK
