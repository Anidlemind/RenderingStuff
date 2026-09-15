#pragma once

#include <cstdint>

#include "app/thread_pool.h"
#include "render/texture.h"
#include "sdl_platform/window.h"
#include "render/frame_buffer.h"
#include "scene/camera.h"
#include "scene/camera_controller.h"
#include "scene/scene.h"

class App {
public:
  App();
  ~App();

  int run();

private:
  void update(float dt);
  void render();

  void packPixels();

  Window           window_;
  FrameBuffer      framebuffer_;
  Camera           camera_;
  CameraController cameraController_;
  Scene            scene_;

  ThreadPool pool_;

  Texture texture_;

  uint64_t lastTime_ = 0;

  float smoothedFps_ = 0.0f;
};