#include "app/options.h"

#include <charconv>
#include <stdexcept>
#include <string_view>

AppOptions ParseOptions(int argc, char** argv, bool headless_only) {
  AppOptions options;
  options.headless = headless_only;
  bool explicit_scene = false;
  // OBJ/MTL supplies the default texture; --texture is an explicit override.
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      options.help = true;
      continue;
    }
    if (arg == "--headless") {
      options.headless = true;
      continue;
    }
    if (arg == "--no-texture") {
      options.texture.clear();
      options.no_texture = true;
      continue;
    }
    if (arg == "--shadows") {
      options.shadows = true;
      continue;
    }
    if (arg == "--no-object-culling") {
      options.object_culling = false;
      continue;
    }
    const auto value = [&]() -> std::string_view {
      if (++i >= argc) {
        throw std::invalid_argument("Missing value for " + std::string(arg));
      }
      return argv[i];
    };
    const auto number = [&](int minimum, int maximum) {
      const auto text = value();
      int result = 0;
      const auto [end, error] =
          std::from_chars(text.data(), text.data() + text.size(), result);
      if (error != std::errc{} || end != text.data() + text.size() ||
          result < minimum || result > maximum) {
        throw std::invalid_argument("Invalid value for " + std::string(arg));
      }
      return result;
    };
    if (arg == "--width") {
      options.width = number(1, 16384);
    } else if (arg == "--height") {
      options.height = number(1, 16384);
    } else if (arg == "--workers") {
      options.workers = number(0, 256);
    } else if (arg == "--tile-size") {
      options.tile_size = number(0, 32);
      if (options.tile_size != 0 && options.tile_size != 16 &&
          options.tile_size != 32) {
        throw std::invalid_argument("Tile size must be 0, 16 or 32");
      }
    } else if (arg == "--shading") {
      options.shading = ParseShadingMode(value());
    } else if (arg == "--filter") {
      options.filter = ParseTextureFilter(value());
    } else if (arg == "--aa") {
      options.antialiasing = ParseAntialiasing(value());
    } else if (arg == "--debug") {
      options.debug_view = ParseDebugView(value());
    } else if (arg == "--shadow-size") {
      options.shadow_size = number(16, 4096);
    } else if (arg == "--frames") {
      options.frames = number(1, 1'000'000);
    } else if (arg == "--scene") {
      const auto name = value();
      if (name == "primitives") {
        options.scene = ScenePreset::kPrimitives;
      } else if (name == "shrimp") {
        options.scene = ScenePreset::kShrimp;
      } else {
        throw std::invalid_argument("Unknown scene: " + std::string(name));
      }
      explicit_scene = true;
    } else if (arg == "--model") {
      options.model = value();
      if (options.model.empty()) {
        throw std::invalid_argument("Model path must not be empty");
      }
    } else if (arg == "--texture") {
      options.texture = value();
      options.no_texture = false;
    } else if (arg == "--output") {
      options.output = value();
    } else {
      throw std::invalid_argument("Unknown option: " + std::string(arg));
    }
  }
  if (!options.model.empty()) {
    if (explicit_scene) {
      throw std::invalid_argument("--model cannot be combined with --scene");
    }
    options.scene = ScenePreset::kModel;
  }
  if (options.scene == ScenePreset::kPrimitives && !options.texture.empty()) {
    throw std::invalid_argument("--texture requires --scene shrimp or --model");
  }
  if (options.antialiasing == Antialiasing::kSsaa4 &&
      (options.width > 8192 || options.height > 8192)) {
    throw std::invalid_argument("SSAA output dimensions exceed 8192");
  }
  if (options.headless && options.frames == 0) {
    options.frames = 1;
  }
  if (!options.output.empty() && !options.headless && !options.help) {
    throw std::invalid_argument("--output requires --headless");
  }
  return options;
}

const char* Usage() {
  return "Usage: renderer [--headless] [options]\n"
         "       renderer_headless [options]\n"
         "  --width N --height N   Resolution (1..16384; default 1920x1080)\n"
         "  --workers N           Worker threads (0=automatic, maximum 256)\n"
         "  --tile-size N         0=horizontal bands (default), 16/32=square "
         "tiles\n"
         "  --frames N            Stop after N frames (headless default: 1)\n"
         "  --shading MODE        phong (default), unlit, legacy\n"
         "  --scene NAME          primitives (default), shrimp\n"
         "  --shadows             Directional shadow map (Phong only)\n"
         "  --shadow-size N       Shadow resolution (16..4096; default 1024)\n"
         "  --debug MODE          none, depth, normals, wireframe, overdraw\n"
         "  --no-object-culling   Disable object-frustum rejection for "
         "comparison\n"
         "  --aa MODE             none (default), ssaa4 (four samples per "
         "pixel)\n"
         "  --filter MODE         trilinear (default), bilinear, nearest; "
         "legacy always uses nearest\n"
         "  --model PATH          Load an OBJ instead of a built-in scene\n"
         "  --texture PATH        Override textures loaded from OBJ/MTL\n"
         "  --no-texture          Disable texture sampling\n"
         "  --output PATH         Save last headless frame as binary PPM\n"
         "  --help                Show this message\n"
         "SDL: WASD/Space/Ctrl + RMB camera; Tab selects object; arrows move "
         "it;\n"
         "     Q/E rotate, PageUp/PageDown scale; F1 debug, F2 shadows, F3 "
         "AA.\n";
}

Scene LoadScene(const AppOptions& options) {
  if (options.scene == ScenePreset::kPrimitives) {
    return MakePrimitivesScene();
  }
  std::string path = options.model;
  if (options.scene == ScenePreset::kShrimp) {
    path = RENDERER_ASSET_DIR "/Shrimp.obj";
  }
  auto mesh = std::make_shared<Mesh>(Mesh::LoadFromObj(path));
  if (mesh->triangles.empty()) {
    throw std::runtime_error("Model contains no triangles");
  }
  std::shared_ptr<const Texture> texture;
  if (!options.texture.empty()) {
    auto loaded = std::make_shared<Texture>(LoadTexture(options.texture));
    if (loaded->pixels.empty()) {
      throw std::runtime_error("Could not load texture: " + options.texture);
    }
    texture = std::move(loaded);
  }
  Scene scene;
  scene.objects.emplace_back(std::move(mesh));
  scene.objects.back().texture = std::move(texture);
  if (options.scene == ScenePreset::kShrimp) {
    scene.objects.back().model = RotationY(Radians(65));
  }
  return scene;
}

Camera MakeCamera(const AppOptions& options) {
  if (options.scene == ScenePreset::kPrimitives) {
    return MakePrimitivesCamera();
  }
  Camera camera;
  camera.position = {0, 0, 3};
  camera.fov_y = Radians(60);
  return camera;
}

RenderSettings MakeRenderSettings(const AppOptions& options) {
  RenderSettings settings;
  settings.tile_size = options.tile_size;
  settings.shading = options.shading;
  settings.filter = options.filter;
  settings.antialiasing = options.antialiasing;
  settings.debug_view = options.debug_view;
  settings.shadows = options.shadows;
  settings.shadow_size = options.shadow_size;
  settings.object_culling = options.object_culling;
  settings.textures = !options.no_texture;
  return settings;
}
