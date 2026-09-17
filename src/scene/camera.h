#ifndef RENDERER_SRC_SCENE_CAMERA_H_
#define RENDERER_SRC_SCENE_CAMERA_H_

#include "math/math.h"

struct Camera {
  Vec3 position{0.0f, 0.0f, 5.0f};
  float yaw = 0.0f;
  float pitch = 0.0f;

  float fov_y = 1.0f;
  float near = 0.1f;
  float far = 100.0f;

  Vec3 Forward() const;
  Vec3 Right() const;
  Vec3 Up() const;

  Mat4 View() const;
  Mat4 Projection(float aspect) const;
  Mat4 ViewProjection(float aspect) const;
};

#endif  // RENDERER_SRC_SCENE_CAMERA_H_
