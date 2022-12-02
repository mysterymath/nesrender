#include "trig.h"

#include <limits.h>

#include "mul.h"

extern const char sin_table_lo[];
extern const char sin_table_hi[];

int32_t sine(unsigned angle) {
  if (angle < PI_OVER_2) {
    uint8_t i = (uint32_t)angle * 256 / PI_OVER_2;
    return sin_table_hi[i] * 256 + sin_table_lo[i];
  }
  if (angle == PI_OVER_2)
    return 1 * 65536;
  if (angle <= PI)
    return sine(PI - angle);
  return -sine(angle - PI);
}

int32_t cosi(unsigned angle) { return sine(PI_OVER_2 - angle); }

int mul_sin(unsigned angle, int val) { return (int)mul_hi(sine(angle), val); }

int mul_cos(unsigned angle, int val) { return mul_sin(PI_OVER_2 - angle, val); }
