#include "mul.h"

#include <stdbool.h>

extern const uint8_t qs_0_255_lo[];
extern const uint8_t qs_0_255_hi[];
extern const uint8_t qs_256_511_lo[];
extern const uint8_t qs_256_511_hi[];

// (uint8_t)floor(n^2/4)
static uint16_t qs_8(uint8_t n, bool hi) {
  return hi ? qs_256_511_lo[n] : qs_0_255_lo[n];
}

// floor(n^2/4)
static uint16_t qs_16(uint8_t n, bool hi) {
  if (hi)
    return qs_256_511_hi[n] << 8 | qs_256_511_lo[n];
  return qs_0_255_hi[n] << 8 | qs_0_255_lo[n];
}

// Quarter-square multiplication is used for the below.

uint8_t half_mul_8(uint8_t a, uint8_t b) {
  if (a < b)
    return half_mul_8(b, a);
  uint8_t sum;
  bool carry = __builtin_add_overflow(a, b, &sum);
  return qs_8(sum, carry) - qs_8(a - b, false);
}

uint16_t full_mul_8(uint8_t a, uint8_t b) {
  if (a < b)
    return full_mul_8(b, a);
  uint8_t sum;
  bool carry = __builtin_add_overflow(a, b, &sum);
  return qs_16(sum, carry) - qs_16(a - b, false);
}

uint16_t mul_hi(uint16_t a, uint16_t b) {
  //                a_hi          a_lo
  // x              b_hi          b_lo
  // =========================================
  //                a_hi * b_lo   a_lo * b_lo
  //   a_hi * b_hi  a_lo * b_hi
  // The low part can only affect the answer by 1, so skip it.

  return full_mul_8(a >> 8, b >> 8) +
         ((full_mul_8(a >> 8, b) + full_mul_8(a, b >> 8)) >> 8);
}
