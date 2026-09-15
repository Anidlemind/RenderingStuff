#include "scene/camera.h"

#include <cmath>

Vec3 Camera::forward() const {
  const float cp = std::cos(pitch);
  return {
    std::sin(yaw) * cp,
    std::sin(pitch),
    -std::cos(yaw) * cp
  };
}

Vec3 Camera::right() const {
  const Vec3 f = forward();
  return normalize(cross(f, {0.0f, 1.0f, 0.0f}));
}

Vec3 Camera::up() const {
  return cross(right(), forward());
}

Mat4 Camera::view() const {
  return lookAt(position, position + forward(), up());
}

Mat4 Camera::projection(float aspect) const {
  return perspective(fovY, aspect, near, far);
}

Mat4 Camera::viewProjection(float aspect) const {
  return projection(aspect) * view();
}