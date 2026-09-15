#pragma once

#include "math/math.h"

struct Camera {
  Vec3  position{0.0f, 0.0f, 5.0f};
  float yaw    = 0.0f;
  float pitch  = 0.0f;

  float fovY   = 1.0f;
  float near   = 0.1f;
  float far    = 100.0f;

  Vec3 forward() const;
  Vec3 right()   const;
  Vec3 up()      const;

  Mat4 view() const;
  Mat4 projection(float aspect) const;
  Mat4 viewProjection(float aspect) const;
};