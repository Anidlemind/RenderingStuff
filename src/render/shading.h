#ifndef RENDERER_SRC_RENDER_SHADING_H_
#define RENDERER_SRC_RENDER_SHADING_H_

#include <string_view>

#include "assets/texture.h"
#include "render/light.h"

enum class ShadingMode { kLegacy, kUnlit, kPhong };
enum class DebugView { kNone, kDepth, kNormals, kWireframe, kOverdraw };
struct ShadowMap;

struct FragmentSettings {
  ShadingMode mode = ShadingMode::kLegacy;
  TextureFilter filter = TextureFilter::kTrilinear;
  Light light;
  Material material;
  Vec3 camera_position;
  DebugView debug_view = DebugView::kNone;
  const ShadowMap* shadow_map = nullptr;
  bool depth_only = false;
};

// Light direction and linear material colors are constant for a draw.
struct PreparedLighting {
  Vec3 direction;
  Vec3 color;
  Vec3 diffuse;
  Vec3 specular;
  bool has_specular = false;
};

PreparedLighting PrepareLighting(const FragmentSettings& settings);
Vec3 ShadeBlinnPhong(Vec3 albedo, Vec3 normal, Vec3 position,
                     const FragmentSettings& settings);
Vec3 ShadeBlinnPhong(Vec3 albedo, Vec3 normal, Vec3 position,
                     const FragmentSettings& settings,
                     const PreparedLighting& lighting,
                     Vec2 shadow_depth_gradient = {});
// Inverse-transpose of the affine model's linear part. Singular transforms
// throw.
Mat4 NormalMatrix(const Mat4& model);
float TextureLod(Vec2 dx, Vec2 dy, int width, int height);
ShadingMode ParseShadingMode(std::string_view name);
TextureFilter ParseTextureFilter(std::string_view name);
DebugView ParseDebugView(std::string_view name);

#endif  // RENDERER_SRC_RENDER_SHADING_H_
