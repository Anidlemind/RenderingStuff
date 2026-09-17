#ifndef RENDERER_SRC_APP_OPTIONS_H_
#define RENDERER_SRC_APP_OPTIONS_H_

#include <string>

#include "render/antialiasing.h"
#include "render/shading.h"
#include "render/software_renderer.h"

enum class ScenePreset { kPrimitives, kShrimp, kModel };

struct AppOptions {
  int width = 1920;
  int height = 1080;
  int workers = 0;
  int tile_size = 0;
  ShadingMode shading = ShadingMode::kPhong;
  TextureFilter filter = TextureFilter::kTrilinear;
  Antialiasing antialiasing = Antialiasing::kNone;
  DebugView debug_view = DebugView::kNone;
  ScenePreset scene = ScenePreset::kPrimitives;
  bool shadows = false;
  bool no_texture = false;
  bool object_culling = true;
  int shadow_size = 1024;
  int frames = 0;
  bool headless = false;
  bool help = false;
  std::string model;
  std::string texture;
  std::string output;
};

AppOptions ParseOptions(int argc, char** argv, bool headless_only);
const char* Usage();
int RunHeadless(const AppOptions& options);
Scene LoadScene(const AppOptions& options);
Camera MakeCamera(const AppOptions& options);
RenderSettings MakeRenderSettings(const AppOptions& options);

#endif  // RENDERER_SRC_APP_OPTIONS_H_
