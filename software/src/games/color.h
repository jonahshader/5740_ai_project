#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <type_traits>
#include <numbers>

namespace color {

/// Generic 3-channel RGB
template <typename T>
using RGB = std::array<T, 3>;

namespace detail {

/// Scale a [0..1] float into either [0..1] (for floats) or [0..255] (for uint8_t)
template <typename Out>
constexpr Out scale_channel(float c) {
  if constexpr (std::is_integral_v<Out>) {
    // clamp to [0,1], multiply and round to nearest integer
    float clamped = std::clamp(c, 0.0f, 1.0f);
    return static_cast<Out>(std::lround(clamped * 255.0f));
  } else {
    // leave in [0,1]
    return static_cast<Out>(c);
  }
}

/// Helper for HSL→RGB
inline float hue2rgb(float p, float q, float t) {
  if (t < 0.0f)
    t += 1.0f;
  if (t > 1.0f)
    t -= 1.0f;
  if (t < 1.0f / 6)
    return p + (q - p) * 6.0f * t;
  if (t < 1.0f / 2)
    return q;
  if (t < 2.0f / 3)
    return p + (q - p) * (2.0f / 3 - t) * 6.0f;
  return p;
}

} // namespace detail

// ――――――――――――――――――――――――――――――――――――――――――――――
//  HSV → RGB
//  H in [0,360), S,V in [0,1]
template <typename T>
RGB<T> hsv_to_rgb(float H, float S, float V) {
  H = std::fmod(H, 360.0f);
  if (H < 0)
    H += 360.0f;
  H /= 60.0f; // sector 0..5
  int i = static_cast<int>(std::floor(H));
  float f = H - i; // fractional part
  float p = V * (1 - S);
  float q = V * (1 - S * f);
  float t = V * (1 - S * (1 - f));

  float rf, gf, bf;
  switch (i) {
    case 0:
      rf = V;
      gf = t;
      bf = p;
      break;
    case 1:
      rf = q;
      gf = V;
      bf = p;
      break;
    case 2:
      rf = p;
      gf = V;
      bf = t;
      break;
    case 3:
      rf = p;
      gf = q;
      bf = V;
      break;
    case 4:
      rf = t;
      gf = p;
      bf = V;
      break;
    default: /*5*/
      rf = V;
      gf = p;
      bf = q;
      break;
  }

  return {detail::scale_channel<T>(rf), detail::scale_channel<T>(gf), detail::scale_channel<T>(bf)};
}

// ――――――――――――――――――――――――――――――――――――――――――――――
//  HSL → RGB
//  H in [0,360), S,L in [0,1]
template <typename T>
RGB<T> hsl_to_rgb(float Hdeg, float S, float L) {
  float H = std::fmod(Hdeg, 360.0f) / 360.0f;
  if (H < 0)
    H += 1.0f;

  float rf, gf, bf;
  if (S == 0.0f) {
    // achromatic (gray)
    rf = gf = bf = L;
  } else {
    float q = (L < 0.5f) ? (L * (1 + S)) : (L + S - L * S);
    float p = 2 * L - q;
    rf = detail::hue2rgb(p, q, H + 1.0f / 3);
    gf = detail::hue2rgb(p, q, H);
    bf = detail::hue2rgb(p, q, H - 1.0f / 3);
  }

  return {detail::scale_channel<T>(rf), detail::scale_channel<T>(gf), detail::scale_channel<T>(bf)};
}

// ――――――――――――――――――――――――――――――――――――――――――――――
//  CIE L*Ch → RGB
//  L in [0,100], C is chroma, Hdeg in [0,360)
template <typename T>
RGB<T> lch_to_rgb(float L, float C, float Hdeg) {
  // 1) LCh → Lab
  float hRad = Hdeg * static_cast<float>(std::numbers::pi) / 180.0f;
  float a = C * std::cos(hRad);
  float b = C * std::sin(hRad);

  // 2) Lab → XYZ (D65)
  const float REF_X = 95.047f, REF_Y = 100.0f, REF_Z = 108.883f;
  auto invf = [](float t) {
    const float d = 6.0f / 29.0f;
    if (t > d)
      return t * t * t;
    return 3 * d * d * (t - 4.0f / 29.0f);
  };
  float fy = (L + 16.0f) / 116.0f;
  float fx = a / 500.0f + fy;
  float fz = fy - b / 200.0f;

  float X = REF_X * invf(fx);
  float Y = REF_Y * invf(fy);
  float Z = REF_Z * invf(fz);

  // 3) XYZ → linear RGB
  float lr = (0.4124f * X + 0.3576f * Y + 0.1805f * Z) / 100.0f;
  float lg = (0.2126f * X + 0.7152f * Y + 0.0722f * Z) / 100.0f;
  float lb = (0.0193f * X + 0.1192f * Y + 0.9505f * Z) / 100.0f;

  // 4) linear → sRGB
  auto lin_to_srgb = [](float u) {
    u = std::clamp(u, 0.0f, 1.0f);
    return (u <= 0.0031308f) ? (12.92f * u) : (1.055f * std::pow(u, 1 / 2.4f) - 0.055f);
  };
  float rf = lin_to_srgb(lr);
  float gf = lin_to_srgb(lg);
  float bf = lin_to_srgb(lb);

  return {detail::scale_channel<T>(rf), detail::scale_channel<T>(gf), detail::scale_channel<T>(bf)};
}

} // namespace color
