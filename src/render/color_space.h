#ifndef RENDERER_SRC_RENDER_COLOR_SPACE_H_
#define RENDERER_SRC_RENDER_COLOR_SPACE_H_

#include <algorithm>
#include <array>

#include "math/math.h"
#include "render/color.h"

inline float SrgbToLinear(uint8_t channel) {
  static const auto table = [] {
    std::array<float, 256> result{};
    for (int i = 0; i < 256; ++i) {
      const float value = i / 255.0f;
      result[i] = value <= 0.04045f ? value / 12.92f
                                    : std::pow((value + 0.055f) / 1.055f, 2.4f);
    }
    return result;
  }();
  return table[channel];
}
inline Vec3 ToLinear(Color color) {
  return {SrgbToLinear(color.r), SrgbToLinear(color.g), SrgbToLinear(color.b)};
}
inline Color ToSrgb(Vec3 color) {
  const auto channel = [](float value) {
    if (!std::isfinite(value)) {
      value = 0;
    }
    value = std::clamp(value, 0.0f, 1.0f);
    const float encoded = value <= 0.0031308f
                              ? 12.92f * value
                              : 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
    return static_cast<uint8_t>(std::clamp(encoded * 255 + 0.5f, 0.0f, 255.0f));
  };
  return {channel(color.x), channel(color.y), channel(color.z)};
}
inline Vec3 Multiply(Vec3 a, Vec3 b) {
  return {a.x * b.x, a.y * b.y, a.z * b.z};
}

#endif  // RENDERER_SRC_RENDER_COLOR_SPACE_H_
