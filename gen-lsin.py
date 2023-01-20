import math


lo = [0] * 256
hi = [0] * 256

for i in range(256):
  ang_rad = i / 256 * math.pi / 2
  ls = round(math.log2(math.cos(ang_rad)) * 2**11)
  if ls < 0:
    ls += 65536
  lo[i] = ls & 0xff
  hi[i] = ls >> 8

print('#include <stdint.h>')
print('const uint8_t lcos_table_lo[] = {', ','.join(map(str, lo)), '};')
print('const uint8_t lcos_table_hi[] = {', ','.join(map(str, hi)), '};')
