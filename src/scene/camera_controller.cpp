#include "scene/camera_controller.h"

#include <algorithm>
#include <cmath>

namespace {
  constexpr float kPitchLimit = 1.5533430f;
  constexpr int   kMaxMouseDelta = 200;
}

void CameraController::update(Camera& camera, const InputState& input, float dt) const {

  if (input.rmbDown) {
    const int dx = std::clamp(input.mouseDX, -kMaxMouseDelta, kMaxMouseDelta);
    const int dy = std::clamp(input.mouseDY, -kMaxMouseDelta, kMaxMouseDelta);

    camera.yaw   += static_cast<float>(dx) * mouseSensitivity;
    camera.pitch -= static_cast<float>(dy) * mouseSensitivity;

    camera.pitch = std::clamp(camera.pitch, -kPitchLimit, kPitchLimit);
  }

  const Vec3 f = camera.forward();
  const Vec3 r = camera.right();
  const Vec3 u = camera.up();

  Vec3 moveDir{0.0f, 0.0f, 0.0f};

  if (input.forward)  moveDir += f;
  if (input.backward) moveDir -= f;
  if (input.right)    moveDir += r;
  if (input.left)     moveDir -= r;
  if (input.up)       moveDir += u;
  if (input.down)     moveDir -= u;

  const float len = moveDir.length();
  if (len > 1e-6f) {
    moveDir /= len;

    float speed = moveSpeed;
    if (input.sprint) {
      speed *= sprintMultiplier;
    }

    camera.position += moveDir * speed * dt;
  }
}