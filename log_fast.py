#!/usr/bin/env python3
import numpy as np
import math

logt_small = [round(math.log2(n)*2048) if n else -32768 for n in range(256)]
logt_coarse = [0]+[round(math.log2(n*256)*2048) for n in range(1,128)]
#print(logt_small)
#print(logt_coarse)

logt_fine = [round(np.mean([math.log2(hi*256+lo)*2048 - logt_coarse[hi] for hi in range(1, 128)])) for lo in range(256)]
#print(logt_fine)

def lin_to_log(n):
    #if n // 256 == 0:
    #    return logt_small[n % 256]
    #return logt_coarse[n // 256] + logt_fine[n % 256]
    return round(math.log2(n)*2048) if n else -32768

# Extracting out the high bit of w as an 8-bit shift afterwards lets us table
# the remaining 3 bits of w with 5 bits of 8.
alogt = [round(2**(i / 32)*256) for i in range(256)]
print(alogt)

def log_to_lin(n):
    if n < -1.5*2048:
        return 0
    if n <= 0:
        return 1
    if n < 8 * 2048:
      return alogt[n >> 6] >> 8
    n -= 8 * 2048
    return alogt[n >> 6]

def approx(n):
    l = lin_to_log(n)
    a = log_to_lin(l)
    print(n, l, a)
    return a

errs = []
for i in range(0, 32768):
	errs.append(approx(i) - i)

print('Mean error:', np.mean(errs))
print('Mean abs error:', np.mean(np.abs(errs)))
print('Std dev error:', np.std(errs))
