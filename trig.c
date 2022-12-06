#include "trig.h"

#include <limits.h>

#include "mul.h"

extern const char sin_table_lo[];
extern const char sin_table_hi[];

int16_t mul_sin(uint16_t angle, int16_t val) {
  if (angle < PI_OVER_2) {
    uint8_t i = (uint32_t)angle * 256 / PI_OVER_2;
    return mul_hi(sin_table_hi[i] * 256 + sin_table_lo[i], val);
  }
  if (angle == PI_OVER_2)
    return val;
  if (angle <= PI)
    return mul_sin(PI - angle, val);
  return -mul_sin(angle - PI, val);
}

int16_t mul_cos(uint16_t angle, int16_t val) {
  return mul_sin(PI_OVER_2 - angle, val);
}
