import numpy as np
import math

def lohi(table):
	return ([x & 0xff for x in table], [x >> 8 for x in table])

logt = [round(np.mean([math.log2(x + 0x100 + y / 256) * 2**11 for y in range(256)])) for x in range(256)]
logt_lo, logt_hi = lohi(logt)

# Note that this table would normally range from [1,2). Multiply by 2**15 for range [2**15, 2**16].
alogt = [round(np.mean([2**(x / 2**8 + y / 2**16) * 2**15 for y in range(256)])) for x in range(256)]
alogt_lo, alogt_hi = lohi(alogt)

print('#include <stdint.h>')

def c_table(name, tbl):
	print(f'const uint8_t {name}[] =', '{', ','.join(map(str, tbl)), '};')

c_table('logt_lo', logt_lo)
c_table('logt_hi', logt_hi)
c_table('alogt_lo', alogt_lo)
c_table('alogt_hi', alogt_hi)
