#pragma once

#include "sdl_platform/input_state.h"
#include "scene/camera.h"

struct CameraController {
  float moveSpeed         = 5.0f;
  float sprintMultiplier  = 3.0f;
  float mouseSensitivity  = 0.003f;

  void update(Camera& camera, const InputState& input, float dt) const;
};