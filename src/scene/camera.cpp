#include "scene/camera.h"

#include <cmath>

Vec3 Camera::Forward() const {
  const float cp = std::cos(pitch);
  return {std::sin(yaw) * cp, std::sin(pitch), -std::cos(yaw) * cp};
}

Vec3 Camera::Right() const {
  const Vec3 f = Forward();
  return Normalize(Cross(f, {0.0f, 1.0f, 0.0f}));
}

Vec3 Camera::Up() const { return Cross(Right(), Forward()); }

Mat4 Camera::View() const {
  return LookAt(position, position + Forward(), Up());
}

Mat4 Camera::Projection(float aspect) const {
  return Perspective(fov_y, aspect, near, far);
}

Mat4 Camera::ViewProjection(float aspect) const {
  return Projection(aspect) * View();
}
