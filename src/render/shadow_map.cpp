#include "render/shadow_map.h"

#include <algorithm>
#include <stdexcept>

Mat4 ShadowViewProjection(const Scene& scene, Vec3 light_direction) {
  const auto direction = Normalize(light_direction);
  if (direction.Length() == 0) {
    throw std::invalid_argument("Shadows require a nonzero light direction");
  }
  const Vec3 up = std::abs(direction.y) > 0.95f ? Vec3{0, 0, 1} : Vec3{0, 1, 0};
  const auto view = LookAt({}, -direction, up);
  Bounds3D all_bounds{}, caster_bounds{};
  bool have_all = false, have_caster = false;
  const auto expand = [](Bounds3D& bounds, bool& initialized, Vec4 p) {
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
      throw std::invalid_argument("Shadow bounds are too large");
    }
    if (!initialized) {
      bounds = {{p.x, p.y, p.z}, {p.x, p.y, p.z}};
      initialized = true;
    }
    bounds.minimum.x = std::min(bounds.minimum.x, p.x);
    bounds.minimum.y = std::min(bounds.minimum.y, p.y);
    bounds.minimum.z = std::min(bounds.minimum.z, p.z);
    bounds.maximum.x = std::max(bounds.maximum.x, p.x);
    bounds.maximum.y = std::max(bounds.maximum.y, p.y);
    bounds.maximum.z = std::max(bounds.maximum.z, p.z);
  };
  for (const auto& object : scene.objects) {
    if (object.Geometry().triangles.empty()) {
      continue;
    }
    const auto light_from_local = view * object.model;
    const auto& bounds = object.Bounds();
    for (int i = 0; i < 8; ++i) {
      const auto p = light_from_local *
                     Vec4{i & 1 ? bounds.maximum.x : bounds.minimum.x,
                          i & 2 ? bounds.maximum.y : bounds.minimum.y,
                          i & 4 ? bounds.maximum.z : bounds.minimum.z, 1};
      expand(all_bounds, have_all, p);
      if (object.casts_shadow) {
        expand(caster_bounds, have_caster, p);
      }
    }
  }
  if (!have_caster) {
    return Mat4::Identity();
  }
  // A directional shadow stays at the caster's light-space XY. A large floor
  // needs depth coverage but must not dilute the useful XY map resolution.
  const auto axis = [](float minimum, float maximum) {
    const double extent = static_cast<double>(maximum) - minimum;
    const double padding = std::max(0.001, extent * 0.02);
    const double span = extent + 2 * padding;
    return Vec2{
        static_cast<float>(2 / span),
        static_cast<float>(-(static_cast<double>(maximum) + minimum) / span)};
  };
  const auto x = axis(caster_bounds.minimum.x, caster_bounds.maximum.x);
  const auto y = axis(caster_bounds.minimum.y, caster_bounds.maximum.y);
  const auto z = axis(all_bounds.minimum.z, all_bounds.maximum.z);
  Mat4 projection;
  projection.At(0, 0) = x.x;
  projection.At(3, 0) = x.y;
  projection.At(1, 1) = y.x;
  projection.At(3, 1) = y.y;
  projection.At(2, 2) = -z.x;
  projection.At(3, 2) = -z.y;
  return projection * view;
}

Vec2 ShadowMap::ReceiverDepthGradient(Vec3 a, Vec3 b, Vec3 c) const {
  if (size <= 0) {
    return {};
  }
  const auto ab = TransformDirection(view_projection, b - a);
  const auto ac = TransformDirection(view_projection, c - a);
  const double nx =
      static_cast<double>(ab.y) * ac.z - static_cast<double>(ab.z) * ac.y;
  const double ny =
      static_cast<double>(ab.z) * ac.x - static_cast<double>(ab.x) * ac.z;
  const double nz =
      static_cast<double>(ab.x) * ac.y - static_cast<double>(ab.y) * ac.x;
  if (nz == 0 || !std::isfinite(nz)) {
    return {};
  }
  // NDC x grows right, while the shadow image's y grows down.
  const Vec2 gradient{static_cast<float>(-2 * nx / (nz * size)),
                      static_cast<float>(2 * ny / (nz * size))};
  if (!std::isfinite(gradient.x) || !std::isfinite(gradient.y)) {
    return {};
  }
  return gradient;
}

float ShadowMap::Visibility(Vec3 position, Vec3 normal, Vec3 light_direction,
                            Vec2 depth_gradient) const {
  const float slope =
      1 - std::max(0.0f, Dot(Normalize(normal), Normalize(light_direction)));
  return VisibilityWithBias(position, bias * (1 + 2 * slope), depth_gradient);
}

float ShadowMap::VisibilityWithBias(Vec3 position, float receiver_bias,
                                    Vec2 depth_gradient) const {
  if (size <= 0 || depth.size() != static_cast<size_t>(size) * size) {
    return 1;
  }
  const auto p = view_projection * Vec4{position.x, position.y, position.z, 1};
  if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
      p.x < -1 || p.x > 1 || p.y < -1 || p.y > 1 || p.z < -1 || p.z > 1) {
    return 1;
  }
  // Texel centers lie at half-integers. Interpolate depth comparisons, never
  // depths: interpolating depths invents geometry across discontinuities.
  const float sample_x = (p.x * 0.5f + 0.5f) * size - 0.5f;
  const float sample_y = (0.5f - p.y * 0.5f) * size - 0.5f;
  const int x = static_cast<int>(std::floor(sample_x));
  const int y = static_cast<int>(std::floor(sample_y));
  const float fraction_x = sample_x - x, fraction_y = sample_y - y;
  // Nine bilinear PCF taps collapse to a separable 4x4 kernel (16 comparisons
  // instead of 36). Each axis sums to three, hence the normalization by nine.
  const float weights_x[] = {1 - fraction_x, 1, 1, fraction_x};
  const float weights_y[] = {1 - fraction_y, 1, 1, fraction_y};
  const float receiver_depth = p.z - receiver_bias;
  float column_depth[4];
  for (int dx = -1; dx <= 2; ++dx) {
    column_depth[dx + 1] =
        receiver_depth + depth_gradient.x * (dx - fraction_x);
  }
  float lit = 0;
  for (int dy = -1; dy <= 2; ++dy) {
    const float row_depth = depth_gradient.y * (dy - fraction_y);
    for (int dx = -1; dx <= 2; ++dx) {
      const int sx = x + dx, sy = y + dy;
      // Compare at the receiver plane's depth at this texel center. Reusing
      // the center pixel's depth makes a sloped surface shadow itself.
      const float sample_depth = column_depth[dx + 1] + row_depth;
      if (sx < 0 || sy < 0 || sx >= size || sy >= size ||
          sample_depth <= depth[static_cast<size_t>(sy) * size + sx]) {
        lit += weights_x[dx + 1] * weights_y[dy + 1];
      }
    }
  }
  return std::clamp(lit / 9, 0.0f, 1.0f);
}
