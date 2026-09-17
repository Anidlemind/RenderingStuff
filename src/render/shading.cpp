#include "render/shading.h"

#include <stdexcept>

#include "render/color_space.h"
#include "render/shadow_map.h"

ShadingMode ParseShadingMode(std::string_view name) {
  if (name == "legacy") {
    return ShadingMode::kLegacy;
  }
  if (name == "unlit") {
    return ShadingMode::kUnlit;
  }
  if (name == "phong") {
    return ShadingMode::kPhong;
  }
  throw std::invalid_argument("Shading must be legacy, unlit or phong");
}
TextureFilter ParseTextureFilter(std::string_view name) {
  if (name == "nearest") {
    return TextureFilter::kNearest;
  }
  if (name == "bilinear") {
    return TextureFilter::kBilinear;
  }
  if (name == "trilinear") {
    return TextureFilter::kTrilinear;
  }
  throw std::invalid_argument("Filter must be nearest, bilinear or trilinear");
}

PreparedLighting PrepareLighting(const FragmentSettings& settings) {
  const auto& specular = settings.material.specular;
  return {Normalize(settings.light.direction), ToLinear(settings.light.color),
          ToLinear(settings.material.diffuse), ToLinear(specular),
          specular.r != 0 || specular.g != 0 || specular.b != 0};
}

Vec3 ShadeBlinnPhong(Vec3 albedo, Vec3 normal, Vec3 position,
                     const FragmentSettings& settings) {
  return ShadeBlinnPhong(albedo, normal, position, settings,
                         PrepareLighting(settings));
}

Vec3 ShadeBlinnPhong(Vec3 albedo, Vec3 normal, Vec3 position,
                     const FragmentSettings& s,
                     const PreparedLighting& lighting,
                     Vec2 shadow_depth_gradient) {
  const auto n = Normalize(normal), l = lighting.direction;
  const float diffuse = std::max(0.0f, Dot(n, l));
  float specular = 0;
  if (lighting.has_specular && diffuse > 0) {
    const auto view = Normalize(s.camera_position - position);
    if (Dot(n, view) > 0) {
      specular = std::pow(std::max(0.0f, Dot(n, Normalize(l + view))),
                          s.material.shininess);
    }
  }
  const auto base = Multiply(albedo, lighting.diffuse);
  const float visibility =
      s.shadow_map ? s.shadow_map->VisibilityWithBias(
                         position, s.shadow_map->bias * (1 + 2 * (1 - diffuse)),
                         shadow_depth_gradient)
                   : 1;
  return base * s.light.ambient +
         Multiply(lighting.color, base * ((1 - s.light.ambient) * diffuse) +
                                      lighting.specular * specular) *
             (s.light.intensity * visibility);
}

DebugView ParseDebugView(std::string_view name) {
  if (name == "none") {
    return DebugView::kNone;
  }
  if (name == "depth") {
    return DebugView::kDepth;
  }
  if (name == "normals") {
    return DebugView::kNormals;
  }
  if (name == "wireframe") {
    return DebugView::kWireframe;
  }
  if (name == "overdraw") {
    return DebugView::kOverdraw;
  }
  throw std::invalid_argument(
      "Debug view must be none, depth, normals, wireframe or overdraw");
}

Mat4 NormalMatrix(const Mat4& model) {
  const Vec3 a{model.At(0, 0), model.At(0, 1), model.At(0, 2)};
  const Vec3 b{model.At(1, 0), model.At(1, 1), model.At(1, 2)};
  const Vec3 c{model.At(2, 0), model.At(2, 1), model.At(2, 2)};
  const auto ca = Cross(b, c), cb = Cross(c, a), cc = Cross(a, b);
  const float determinant = Dot(a, ca);
  if (!std::isfinite(determinant) || determinant == 0) {
    throw std::invalid_argument("Singular normal transform");
  }
  Mat4 result;
  const Vec3 columns[]{ca / determinant, cb / determinant, cc / determinant};
  for (int i = 0; i < 3; ++i) {
    if (!std::isfinite(columns[i].x) || !std::isfinite(columns[i].y) ||
        !std::isfinite(columns[i].z)) {
      throw std::invalid_argument("Invalid normal transform");
    }
    result.At(i, 0) = columns[i].x;
    result.At(i, 1) = columns[i].y;
    result.At(i, 2) = columns[i].z;
  }
  return result;
}

float TextureLod(Vec2 dx, Vec2 dy, int width, int height) {
  const float footprint = std::max(std::hypot(dx.x * width, dx.y * height),
                                   std::hypot(dy.x * width, dy.y * height));
  return std::log2(std::max(1.0f, footprint));
}
