#ifndef LOG_H
#define LOG_H

#include <stdint.h>

struct Log {
  bool sign;
  int16_t exp;

  Log() = default;
  Log(int16_t val);
  constexpr Log(bool sign, int16_t exp) : sign(sign), exp(exp){};

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

#endif // not LOG_H
