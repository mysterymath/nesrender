import numpy as np
import math

logt = [round(np.mean([math.log2(x + 0x100 + y / 256) * 2**11 for y in range(256)])) for x in range(256)]
print(logt)

# Note that this table would normally range from [1,2). Multiply by 2**15 for range [2**15, 2**16].
alogt = [round(np.mean([2**(x / 2**8 + y / 2**16) * 2**15 for y in range(256)])) for x in range(256)]
print(alogt)

def approx(n):
	orig_n = n

	# Shift the number while keeping its value the same by considering it a
	# fixed-point fraction over 2^m. Stop once of the form 00000001 xxxxxxxx.

	l = 0
	if n & 0xff00:
		while n & 0xff00 != 0x0100:
			n >>= 1
			l += 2**11
	else:
		while n & 0xff00 != 0x0100:
			n <<= 1
			l -= 2**11

	l += logt[n & 0x00ff]

	# back = 2 ** (l / 2**11)
	# back = 2 ** (w + f / 2**11)  = 2**(f/2**11) * 2**w
	# back = 2*(f/2**10) << w

	sp = l >> 3
	w = sp >> 8
	f = sp & 0xff

	back = alogt[f]

	shift = 15 - w
	if shift >= 8:
		back >>= 8
		shift -= 8
	back >>= shift

	print(orig_n, n, l, w, f, back)
	return back

errs = []
for i in range(1, 65536):
	errs.append(approx(i) - i)

print('Mean error:', np.mean(errs))
print('Mean abs error:', np.mean(np.abs(errs)))
print('Std dev error:', np.std(errs))
