print('__attribute__((section(".chr_rom"), used)) const char chr_rom[] = {')

rom = [0] * 4096

# Each byte represents 4 pixels, from low to high: (x,y),(x,y+1),(x+1,y),(x+1,y+1).
for i in range(256):
    # Bit 0 of (x,y)
    if i & 1 << 0:
        rom[i << 4 | 0x0] |= 0xf0
        rom[i << 4 | 0x1] |= 0xf0
        rom[i << 4 | 0x2] |= 0xf0
        rom[i << 4 | 0x3] |= 0xf0
    # Bit 1 of (x,y)
    if i & 1 << 1:
        rom[i << 4 | 0x8] |= 0xf0
        rom[i << 4 | 0x9] |= 0xf0
        rom[i << 4 | 0xa] |= 0xf0
        rom[i << 4 | 0xb] |= 0xf0
    # Bit 0 of (x,y+1)
    if i & 1 << 2:
        rom[i << 4 | 0x4] |= 0xf0
        rom[i << 4 | 0x5] |= 0xf0
        rom[i << 4 | 0x6] |= 0xf0
        rom[i << 4 | 0x7] |= 0xf0
    # Bit 1 of (x,y+1)
    if i & 1 << 3:
        rom[i << 4 | 0xc] |= 0xf0
        rom[i << 4 | 0xd] |= 0xf0
        rom[i << 4 | 0xe] |= 0xf0
        rom[i << 4 | 0xf] |= 0xf0
    # Bit 0 of (x+1,y)
    if i & 1 << 4:
        rom[i << 4 | 0x0] |= 0x0f
        rom[i << 4 | 0x1] |= 0x0f
        rom[i << 4 | 0x2] |= 0x0f
        rom[i << 4 | 0x3] |= 0x0f
    # Bit 1 of (x+1,y)
    if i & 1 << 5:
        rom[i << 4 | 0x8] |= 0x0f
        rom[i << 4 | 0x9] |= 0x0f
        rom[i << 4 | 0xa] |= 0x0f
        rom[i << 4 | 0xb] |= 0x0f
    # Bit 0 of (x+1,y+1)
    if i & 1 << 6:
        rom[i << 4 | 0x4] |= 0x0f
        rom[i << 4 | 0x5] |= 0x0f
        rom[i << 4 | 0x6] |= 0x0f
        rom[i << 4 | 0x7] |= 0x0f
    # Bit 1 of (x+1,y+1)
    if i & 1 << 7:
        rom[i << 4 | 0xc] |= 0x0f
        rom[i << 4 | 0xd] |= 0x0f
        rom[i << 4 | 0xe] |= 0x0f
        rom[i << 4 | 0xf] |= 0x0f

for i in rom:
    print(hex(i) + ',')
print('};')
