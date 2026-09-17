#include "app/app.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <stdexcept>

#include "ui/text.h"

App::App(const AppOptions& options)
    : window_("renderer", options.width, options.height),
      framebuffer_(options.width, options.height),
      camera_(MakeCamera(options)),
      scene_(LoadScene(options)),
      frame_limit_(options.frames),
      renderer_(options.workers) {
  settings_ = MakeRenderSettings(options);
}

int App::Run() {
  using Clock = std::chrono::steady_clock;
  auto last = Clock::now();
  float smoothed_fps = 0;
  unsigned frames = 0;
  while (window_.IsOpen() &&
         (frame_limit_ == 0 || frames < static_cast<unsigned>(frame_limit_))) {
    window_.PollEvents();
    if (!window_.IsOpen()) {
      break;
    }
    const auto now = Clock::now();
    const float dt = std::chrono::duration<float>(now - last).count();
    last = now;
    if (dt > 1e-6f) {
      const float instant = 1 / dt;
      const float alpha = std::min(1.0f, dt * 5);
      smoothed_fps = smoothed_fps <= 0
                         ? instant
                         : smoothed_fps * (1 - alpha) + instant * alpha;
    }
    camera_controller_.Update(camera_, window_.Input(), std::min(dt, 0.1f));
    const auto& input = window_.Input();
    if (input.next_object) {
      selected_object_ = (selected_object_ + 1) % scene_.objects.size();
    }
    if (input.next_debug) {
      settings_.debug_view = static_cast<DebugView>(
          (static_cast<int>(settings_.debug_view) + 1) % 5);
    }
    if (input.toggle_shadows) {
      settings_.shadows = !settings_.shadows;
    }
    if (input.toggle_aa && framebuffer_.Width() <= 8192 &&
        framebuffer_.Height() <= 8192) {
      settings_.antialiasing = settings_.antialiasing == Antialiasing::kNone
                                   ? Antialiasing::kSsaa4
                                   : Antialiasing::kNone;
    }
    auto& model = scene_.objects[selected_object_].model;
    const float step = std::min(dt, 0.1f);
    const Vec3 movement{
        static_cast<float>(input.object_right - input.object_left), 0,
        static_cast<float>(input.object_backward - input.object_forward)};
    const float scale = std::exp((input.grow - input.shrink) * step);
    const float current_scale = TransformDirection(model, {1, 0, 0}).Length();
    const float bounded_scale =
        std::clamp(current_scale * scale, 0.01f, 100.0f) / current_scale;
    model = Translation(movement * step * 2) * model *
            RotationY((input.rotate_right - input.rotate_left) * step) *
            Scale(bounded_scale);
    const auto stats =
        renderer_.Render(scene_, camera_, settings_, framebuffer_);
    char label[128];
    std::snprintf(label, sizeof(label), "FPS: %d OBJ: %zu/%zu CULL: %zu",
                  static_cast<int>(smoothed_fps + 0.5f), selected_object_ + 1,
                  stats.input_objects, stats.culled_objects);
    DrawText(framebuffer_, 11, 11, label, {0, 0, 0});
    DrawText(framebuffer_, 10, 10, label, {255, 255, 255});
    window_.Present(framebuffer_.Pixels(), window_.Width(), window_.Height());
    if (++frames % 30 == 0) {
      std::fprintf(stderr,
                   "clear %.1f shadow %.1f transform %.1f setup %.1f raster "
                   "%.1f resolve %.1f ms\n",
                   stats.clear_ms, stats.shadow_ms, stats.transform_ms,
                   stats.setup_ms, stats.raster_ms, stats.resolve_ms);
    }
  }
  return 0;
}
