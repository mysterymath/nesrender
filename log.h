#ifndef LOG_H
#define LOG_H

#include "gcem.hpp"
#include <stdint.h>

struct Log {
  bool sign;
  int16_t exp;

  Log() = default;
  Log(int16_t val);
  template <typename T> Log(T val) : Log(static_cast<int16_t>(val)) {}
  constexpr Log(bool sign, int16_t exp) : sign(sign), exp(exp){};
  explicit constexpr Log(float val);

  static constexpr Log pow2(uint8_t k) { return Log(false, k << 11); }
  static constexpr Log zero() { return Log(false, -32768); }
  static constexpr Log one() { return Log(false, 0); }

  operator int16_t() const;
  constexpr Log abs() const { return Log(false, exp); }

  Log operator-() const;
  Log operator*(const Log &other) const;
  Log &operator*=(const Log &other);
  Log operator/(const Log &other) const;
  Log &operator/=(const Log &other);

  bool operator<(const Log &other) const;
  bool operator<=(const Log &other) const;
  bool operator>(const Log &other) const;
  bool operator>=(const Log &other) const;
  bool operator==(const Log &other) const;
  bool operator!=(const Log &other) const;
};

constexpr Log::Log(float val) : sign(false), exp(0) {
  if (val == 0) {
    exp = -32768;
    return;
  }

  if (val < 0) {
    val = -val;
    sign = true;
  }

  float fexp = gcem::round(gcem::log2(val) * 2048);
  if (fexp < -32768)
    fexp = -32768;
  if (fexp >= 15 * 2048)
    fexp = 15 * 2048 - 1;
}

#endif // not LOG_H
