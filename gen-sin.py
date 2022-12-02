import math


lo = [0] * 256
hi = [0] * 256

for i in range(256):
  ang_rad = i / 256 * math.pi / 2
  s = round(math.sin(ang_rad) * 2**16)
  lo[i] = s & 0xff
  hi[i] = s >> 8

print('const char sin_table_lo[] = {', ','.join(map(str, lo)), '};')
print('const char sin_table_hi[] = {', ','.join(map(str, hi)), '};')
