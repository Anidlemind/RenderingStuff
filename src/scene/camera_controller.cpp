#include "scene/camera_controller.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPitchLimit = 1.5533430f;
constexpr int kMaxMouseDelta = 200;
}  // namespace

void CameraController::Update(Camera& camera, const InputState& input,
                              float dt) const {
  if (input.rmb_down) {
    const int dx = std::clamp(input.mouse_dx, -kMaxMouseDelta, kMaxMouseDelta);
    const int dy = std::clamp(input.mouse_dy, -kMaxMouseDelta, kMaxMouseDelta);

    camera.yaw += static_cast<float>(dx) * mouse_sensitivity;
    camera.pitch -= static_cast<float>(dy) * mouse_sensitivity;

    camera.pitch = std::clamp(camera.pitch, -kPitchLimit, kPitchLimit);
  }

  const Vec3 f = camera.Forward();
  const Vec3 r = camera.Right();
  const Vec3 u = camera.Up();

  Vec3 move_dir{0.0f, 0.0f, 0.0f};

  if (input.forward) {
    move_dir += f;
  }
  if (input.backward) {
    move_dir -= f;
  }
  if (input.right) {
    move_dir += r;
  }
  if (input.left) {
    move_dir -= r;
  }
  if (input.up) {
    move_dir += u;
  }
  if (input.down) {
    move_dir -= u;
  }

  const float len = move_dir.Length();
  if (len > 1e-6f) {
    move_dir /= len;

    float speed = move_speed;
    if (input.sprint) {
      speed *= sprint_multiplier;
    }

    camera.position += move_dir * speed * dt;
  }
}
