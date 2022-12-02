import math

lo = [0] * 512
hi = [0] * 512

for i in range(512):
  qs = math.floor((i * i) / 4)
  lo[i] = qs & 0xff
  hi[i] = qs >> 8

print('const char qs_0_255_lo[] = {', ','.join(map(str, lo[:256])), '};')
print('const char qs_0_255_hi[] = {', ','.join(map(str, hi[:256])), '};')
print('const char qs_256_511_lo[] = {', ','.join(map(str, lo[256:])), '};')
print('const char qs_256_511_hi[] = {', ','.join(map(str, hi[256:])), '};')
