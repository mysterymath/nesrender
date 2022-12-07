#ifndef MUL_H
#define MUL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

uint8_t half_mul_8(uint8_t a, uint8_t b);
uint16_t full_mul_8(uint8_t a, uint8_t b);
uint16_t mul_hi(uint16_t a, uint16_t b);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // not MUL_H
