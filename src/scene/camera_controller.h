#ifndef RENDERER_SRC_SCENE_CAMERA_CONTROLLER_H_
#define RENDERER_SRC_SCENE_CAMERA_CONTROLLER_H_

#include "scene/camera.h"
#include "sdl_platform/input_state.h"

struct CameraController {
  float move_speed = 5.0f;
  float sprint_multiplier = 3.0f;
  float mouse_sensitivity = 0.003f;

  void Update(Camera& camera, const InputState& input, float dt) const;
};

#endif  // RENDERER_SRC_SCENE_CAMERA_CONTROLLER_H_
