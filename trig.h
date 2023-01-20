#ifndef TRIG_H
#define TRIG_H

#include <stdint.h>

#include "log.h"

#define PI_OVER_2 (65536 / 4)
#define PI (65536 / 2)
#define PI3_OVER_2 (65536 / 4 * 3)
#define PI2 65536

int16_t mul_sin(uint16_t angle, int16_t val);
int16_t mul_cos(uint16_t angle, int16_t val);

Log lsin(uint16_t angle);
Log lcos(uint16_t angle);

#endif // not TRIG_H
