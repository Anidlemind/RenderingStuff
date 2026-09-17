#ifndef RENDERER_SRC_RENDER_SHADOW_MAP_H_
#define RENDERER_SRC_RENDER_SHADOW_MAP_H_

#include <vector>

#include "math/math.h"
#include "scene/scene.h"

struct ShadowMap {
  Mat4 view_projection;
  int size = 0;
  float bias = 0.002f;
  std::vector<float> depth;
  // Receiver depth change per shadow texel; prepared once per triangle.
  Vec2 ReceiverDepthGradient(Vec3 a, Vec3 b, Vec3 c) const;
  // Bilinearly filtered 3x3 PCF; samples outside the map are lit.
  float Visibility(Vec3 position, Vec3 normal, Vec3 light_direction,
                   Vec2 depth_gradient = {}) const;
  // Reuse the bias when the caller has already evaluated the light angle.
  float VisibilityWithBias(Vec3 position, float receiver_bias,
                           Vec2 depth_gradient) const;
};

// Fit XY to casters and depth to the whole scene, including off-camera objects.
Mat4 ShadowViewProjection(const Scene& scene, Vec3 light_direction);

#endif  // RENDERER_SRC_RENDER_SHADOW_MAP_H_
