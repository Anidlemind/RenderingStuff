#ifndef RENDERER_SRC_APP_APP_H_
#define RENDERER_SRC_APP_APP_H_

#include "app/options.h"
#include "render/software_renderer.h"
#include "scene/camera_controller.h"
#include "sdl_platform/window.h"

class App {
 public:
  explicit App(const AppOptions& options);
  int Run();

 private:
  Window window_;
  FrameBuffer framebuffer_;
  Camera camera_;
  CameraController camera_controller_;
  Scene scene_;
  size_t selected_object_ = 0;
  int frame_limit_;
  RenderSettings settings_;
  SoftwareRenderer renderer_;
};

#endif  // RENDERER_SRC_APP_APP_H_
