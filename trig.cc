#include "trig.h"

#include <limits.h>

#include "mul.h"
#include <stdio.h>

extern const uint8_t sin_table_lo[];
extern const uint8_t sin_table_hi[];
extern const uint8_t lcos_table_lo[];
extern const uint8_t lcos_table_hi[];

int16_t mul_sin(uint16_t angle, int16_t val) {
  if (angle < PI_OVER_2) {
    uint8_t i = (uint32_t)angle * 256 / PI_OVER_2;
    uint16_t s = sin_table_hi[i] * 256 + sin_table_lo[i];
    return val < 0 ? -mul_hi(s, -val) : mul_hi(s, val);
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

Log lsin(uint16_t angle) { return lcos(PI_OVER_2 - angle); }
Log lcos(uint16_t angle) {
  bool sign = false;
  if (angle > PI) {
    angle -= PI;
    sign = true;
  }
  if (angle > PI_OVER_2) {
    sign = !sign;
    angle = PI - angle;
  } else if (angle == PI_OVER_2) {
    return Log::zero();
  }

  // rads = angle / 65536 * 2PI
  // i = rads / PI/2 * 256
  // i = angle / 65536 * 2PI / PI/2 * 256
  // i = angle / 65536 * 4 * 256
  // i = angle >> 6
  uint8_t i = angle >> 6;
  return Log(sign, lcos_table_hi[i] << 8 | lcos_table_lo[i]);
}
