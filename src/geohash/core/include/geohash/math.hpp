#pragma once
#include <cstdint>

namespace geohash {

// IEEE-754 Floating Point parts
struct FloatingPointParts {
  uint64_t mantissa : 52;
  uint64_t exponent : 11;
  uint64_t sign : 1;
};

// IEEE-754 Floating Point
union FloatingPoint {
  double value;
  FloatingPointParts parts;
};

// Fast calculation of 2^n
inline constexpr auto power2(const int32_t exponent) -> double {
  FloatingPoint floating_point{2};
  floating_point.parts.exponent += static_cast<uint64_t>(exponent - 1);
  return floating_point.value;
}

// Fast calculation of 10^n
inline constexpr auto power10(int32_t exponent) -> double {
  auto result = 1.0;
  auto base = 10.0;
  bool inv = exponent < 0;
  if (inv) {
    exponent = -exponent;
  }
  while (exponent) {
    if (exponent & 1) {
      result *= base;
    }
    exponent >>= 1U;
    base *= base;
  }
  return inv ? 1.0 / result : result;
}

}  // namespace geohash
