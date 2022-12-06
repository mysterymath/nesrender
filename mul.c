#include "mul.h"

#include <stdbool.h>

extern const char qs_0_255_lo[];
extern const char qs_0_255_hi[];
extern const char qs_256_511_lo[];
extern const char qs_256_511_hi[];

// (char)floor(n^2/4)
static unsigned qs_8(char n, bool hi) {
  return hi ? qs_256_511_lo[n] : qs_0_255_lo[n];
}

// floor(n^2/4)
static unsigned qs_16(char n, bool hi) {
  if (hi)
    return qs_256_511_hi[n] << 8 | qs_256_511_lo[n];
  return qs_0_255_hi[n] << 8 | qs_0_255_lo[n];
}

// Quarter-square multiplication is used for the below.

char half_mul_8(char a, char b) {
  if (a < b)
    return half_mul_8(b, a);
  char sum;
  bool carry = __builtin_add_overflow(a, b, &sum);
  return qs_8(sum, carry) - qs_8(a - b, false);
}

unsigned full_mul_8(char a, char b) {
  if (a < b)
    return full_mul_8(b, a);
  char sum;
  bool carry = __builtin_add_overflow(a, b, &sum);
  return qs_16(sum, carry) - qs_16(a - b, false);
}

unsigned mul_hi(unsigned a, unsigned b) {
  //                a_hi          a_lo
  // x              b_hi          b_lo
  // =========================================
  //                a_hi * b_lo   a_lo * b_lo
  //   a_hi * b_hi  a_lo * b_hi
  // The low part can only affect the answer by 1, so skip it.

  return full_mul_8(a >> 8, b >> 8) +
         ((full_mul_8(a >> 8, b) + full_mul_8(a, b >> 8)) >> 8);
}
