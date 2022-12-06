#ifndef MUL_H
#define MUL_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

char half_mul_8(char a, char b);
unsigned full_mul_8(char a, char b);
unsigned mul_hi(unsigned a, unsigned b);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // not MUL_H
