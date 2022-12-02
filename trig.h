#ifndef TRIG_H
#define TRIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define PI_OVER_2 (65536 / 4)
#define PI (65536 / 2)
#define PI3_OVER_2 (65536 / 4 * 3)
#define PI2 65536

int32_t sine(unsigned angle);
int32_t cosi(unsigned angle);
int mul_sin(unsigned angle, int val);
int mul_cos(unsigned angle, int val);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // not TRIG_H
