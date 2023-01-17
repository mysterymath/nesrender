#include "log.h"

extern "C" {
extern const uint8_t logt_lo[];
extern const uint8_t logt_hi[];
extern const uint8_t alogt_lo[];
extern const uint8_t alogt_hi[];
}

Log::Log(int16_t val) : sign(val < 0), exp(0) {
  if (val == 0) {
    exp = -32768;
    return;
  }
  if (val == -32768) {
    exp = 32767;
    return;
  }

  uint16_t v = val < 0 ? -val : val;
  if (v & 0xff00) {
    while (v >> 8 != 1) {
      v >>= 1;
      exp += 2048;
    }
  } else {
    while (v >> 8 != 1) {
      v <<= 1;
      exp -= 2048;
    }
  }

  exp += logt_lo[v & 0xff] | logt_hi[v & 0xff] << 8;
}

Log::operator int16_t() const {
  // 2^(-1) = 1/2, which, rounding to even, should round to zero. All negative
  // numbers above that should round to 1.
  if (exp <= -1 * 2048)
    return 0;
  if (exp <= 0)
    return 1;

  // Exponents that are too large are clipped to max.
  if (exp >= 15 * 2048)
    return sign ? -32768 : 32767;

  // Special case powers of two; these can be easily made exact, while the table
  // solution is approximate.
  if (!(exp & 0x07ff)) {
    int16_t uval = 1 << (exp >> 11);
    return sign ? -uval : uval;
  }

  // Shift the exponent so the whole/frac boundary is along the byte.
  uint16_t split = exp;
  split >>= 3;
  uint8_t whole = split >> 8;
  uint8_t frac = split & 0xff;

  // Produce the unsigned result, but shifted up by 15.
  uint16_t uresult = alogt_lo[frac] | alogt_hi[frac] << 8;

  // Shift the result back to produce the final unsigned result.
  uint8_t shift = 15 - whole;
#if 0
  if (shift >= 8) {
    shift -= 8;
    uresult >>= 8;
  }
#endif
  uresult >>= shift;

  return sign ? -uresult : uresult;
}

Log Log::operator-() const { return Log(!sign, exp); }
Log Log::operator*(const Log &other) const { return (Log)(*this) *= other; }
Log &Log::operator*=(const Log &other) {
  if (other.sign)
    sign = !sign;
  if (exp == -32768 || other.exp == -32768)
    exp = -32768;
  else
    exp += other.exp;
  return *this;
}
Log Log::operator/(const Log &other) const { return (Log)(*this) /= other; }
Log &Log::operator/=(const Log &other) {
  if (other.sign)
    sign = !sign;
  if (exp == -32768 || other.exp == -32768)
    exp = -32768;
  else
    exp -= other.exp;
  return *this;
}

bool Log::operator<(const Log &other) const {
  if (sign != other.sign)
    return sign;
  return (exp < other.exp) ^ sign;
}
bool Log::operator<=(const Log &other) const { return !(*this > other); }
bool Log::operator>(const Log &other) const {
  if (sign != other.sign)
    return !sign;
  return (exp > other.exp) ^ sign;
}
bool Log::operator>=(const Log &other) const { return !(*this < other); }
bool Log::operator==(const Log &other) const {
  return sign == other.sign && exp == other.exp;
}
bool Log::operator!=(const Log &other) const { return !(*this == other); }
