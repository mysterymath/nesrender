.include "nes.inc"

.zeropage frame_count

.section .nmi.100,"axR",@progbits
  inc frame_count 

  lda PPUSTATUS

  ; Disable rendering; we need slightly more than one vblank.
  lda #0
  sta PPUMASK

  ; Run the framebuffer copy routine.
  lda #>($2000 + 32)
  sta PPUADDR
  lda #<($2000 + 32)
  sta PPUADDR
  jsr framebuffer

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

  ; Re-enable rendering.
  lda #0b00011110
  sta PPUMASK
