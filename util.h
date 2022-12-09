#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

template <typename T> T abs(T t) { return t < 0 ? -t : t; }

template <typename T> T rotl(T t, uint8_t amt) {
  return t << amt | t >> (sizeof(T) * 8 - amt);
}

#endif // not UTIL_H
