// fixed point implementation, written by Claude 3.7

#pragma once

#include <cstdint>
#include <type_traits>
#include <cmath>

namespace jnb {

template <typename T, int FractionalBits> class FixedPoint {
private:
  T value;

  // Helper type for wider intermediate calculations
  using WideType = typename std::conditional<
      std::is_same<T, int8_t>::value, int16_t,
      typename std::conditional<std::is_same<T, int16_t>::value, int32_t,
                                typename std::conditional<std::is_same<T, int32_t>::value, int64_t,
                                                          T>::type>::type>::type;

public:
  // Constants
  static constexpr T FRACTIONAL_MASK = (1 << FractionalBits) - 1;
  static constexpr T ONE = (1 << FractionalBits);

  // Constructors
  constexpr FixedPoint() : value(0) {}
  constexpr explicit FixedPoint(T integer) : value(integer << FractionalBits) {}
  constexpr explicit FixedPoint(float floating)
      : value(static_cast<T>(std::round(floating * ONE))) {}

  // Special constructor for raw values (internal use)
  constexpr static FixedPoint from_raw(T raw_value) {
    FixedPoint result;
    result.value = raw_value;
    return result;
  }

  // Conversion methods
  constexpr T raw_value() const {
    return value;
  }
  constexpr T to_integer_floor() const {
    return value >> FractionalBits;
  }
  constexpr T to_integer_ceil() const {
    // If there is any fractional part, round up
    T integer_part = value >> FractionalBits;
    return (fractional_part() > 0) ? integer_part + 1 : integer_part;
  }
  constexpr T to_integer_rounded() const {
    return (value + (ONE >> 1)) >> FractionalBits;
  }
  constexpr float to_float() const {
    return static_cast<float>(value) / ONE;
  }
  constexpr double to_double() const {
    return static_cast<double>(value) / ONE;
  }

  // Fractional part (returns the fractional part as a raw value)
  constexpr T fractional_part() const {
    return value & FRACTIONAL_MASK;
  }

  // Arithmetic operators
  constexpr FixedPoint operator+(const FixedPoint &other) const {
    return from_raw(value + other.value);
  }

  constexpr FixedPoint operator-(const FixedPoint &other) const {
    return from_raw(value - other.value);
  }

  constexpr FixedPoint operator-() const {
    return from_raw(-value);
  }

  constexpr FixedPoint operator*(const FixedPoint &other) const {
    // Use wider type for intermediate calculation to prevent overflow
    WideType result = (static_cast<WideType>(value) * other.value) >> FractionalBits;
    return from_raw(static_cast<T>(result));
  }

  constexpr FixedPoint operator/(const FixedPoint &other) const {
    // Pre-shift to maintain precision, then divide
    WideType result = (static_cast<WideType>(value) << FractionalBits) / other.value;
    return from_raw(static_cast<T>(result));
  }

  // Compound assignment operators
  constexpr FixedPoint &operator+=(const FixedPoint &other) {
    value += other.value;
    return *this;
  }

  constexpr FixedPoint &operator-=(const FixedPoint &other) {
    value -= other.value;
    return *this;
  }

  constexpr FixedPoint &operator*=(const FixedPoint &other) {
    WideType result = (static_cast<WideType>(value) * other.value) >> FractionalBits;
    value = static_cast<T>(result);
    return *this;
  }

  constexpr FixedPoint &operator/=(const FixedPoint &other) {
    WideType result = (static_cast<WideType>(value) << FractionalBits) / other.value;
    value = static_cast<T>(result);
    return *this;
  }

  // Comparison operators
  constexpr bool operator==(const FixedPoint &other) const {
    return value == other.value;
  }

  constexpr bool operator!=(const FixedPoint &other) const {
    return value != other.value;
  }

  constexpr bool operator<(const FixedPoint &other) const {
    return value < other.value;
  }

  constexpr bool operator>(const FixedPoint &other) const {
    return value > other.value;
  }

  constexpr bool operator<=(const FixedPoint &other) const {
    return value <= other.value;
  }

  constexpr bool operator>=(const FixedPoint &other) const {
    return value >= other.value;
  }

  // Utility functions
  constexpr FixedPoint abs() const {
    return from_raw(value >= 0 ? value : -value);
  }

  // Integer scaling (without changing the fixed-point representation)
  constexpr FixedPoint operator*(T scalar) const {
    return from_raw(value * scalar);
  }

  constexpr FixedPoint operator/(T scalar) const {
    return from_raw(value / scalar);
  }
};

// Integer multiplication (for when you want to scale a fixed point)
template <typename T, int F>
constexpr FixedPoint<T, F> operator*(T scalar, const FixedPoint<T, F> &fixed) {
  return fixed * scalar;
}

// Math functions
template <typename T, int F> constexpr FixedPoint<T, F> sqrt(const FixedPoint<T, F> &x) {
  if (x.raw_value() <= 0)
    return FixedPoint<T, F>();

  // Initial guess (for faster convergence)
  FixedPoint<T, F> guess = FixedPoint<T, F>::from_raw(x.raw_value() >> 1);
  if (guess.raw_value() == 0)
    guess = FixedPoint<T, F>(1);

  // Newton-Raphson method
  for (int i = 0; i < 8; ++i) { // Usually converges quickly
    guess = (guess + x / guess) / FixedPoint<T, F>(2);
  }

  return guess;
}

} // namespace jnb
