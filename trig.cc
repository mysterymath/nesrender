#include "trig.h"

#include <limits.h>

#include <stdio.h>

extern const uint8_t lcos_table_lo[];
extern const uint8_t lcos_table_hi[];

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
