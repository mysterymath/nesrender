import numpy as np
import math

def approx(n):
	orig_n = n

	#l = math.log2(n)
	#return 2**l

	# n = 2**a * 2**b, where b < 1

	# Shift the number left by m while keeping its value the same by considering
	# it a fixed-point fraction over 2^m. Stop once the high bit is high.

	m = 0
	while not n & 0x8000:
		m += 1
		n <<= 1

	# n = n' / 2^m
	# log n = log n' - m
	l = math.log2(n) - m

	# back = 2 ** (l / 2**10)
	# back = 2 ** (w + f / 2**10)  = 2**(f/2**10) * 2**w
	# back = 2*(f/2**10) << w

	l *= 2**11
	l = round(l)
	back = l / 2**11
	back = round(2**back)
	print(orig_n, n, m, l, back)
	return back

errs = []
for i in range(1, 65536):
	errs.append(approx(i) - i)

print('Mean error:', np.mean(errs))
print('Mean abs error:', np.mean(np.abs(errs)))
print('Std dev error:', np.std(errs))
