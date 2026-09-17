#include "scene/benchmark_scenes.h"

#include <stdexcept>

namespace {
void Quad(Mesh& scene, float left, float bottom, float right, float top,
          float left_z, float right_z, Color color) {
  const int offset = static_cast<int>(scene.vertices.size());
  scene.vertices.push_back({left, bottom, left_z, color, {0, 0, 1}, {0, 0}});
  scene.vertices.push_back({right, bottom, right_z, color, {0, 0, 1}, {1, 0}});
  scene.vertices.push_back({right, top, right_z, color, {0, 0, 1}, {1, 1}});
  scene.vertices.push_back({left, top, left_z, color, {0, 0, 1}, {0, 1}});
  scene.triangles.push_back({offset, offset + 1, offset + 2});
  scene.triangles.push_back({offset, offset + 2, offset + 3});
}
}  // namespace

BenchmarkScene MakeBenchmarkScene(std::string_view name) {
  BenchmarkScene fixture;
  fixture.camera.position = {0, 0, 0};
  fixture.camera.fov_y = Radians(90);
  fixture.camera.near = 1;
  fixture.camera.far = 20;
  fixture.settings.ambient = 1;
  fixture.settings.clear_color = {};
  if (name == "quad") {
    Quad(fixture.mesh, -1.5f, -1, 1.5f, 1, -2, -5, {255, 255, 255});
    fixture.texture.width = fixture.texture.height = 32;
    for (int y = 0; y < 32; ++y) {
      for (int x = 0; x < 32; ++x) {
        fixture.texture.pixels.push_back(
            ((x / 4 + y / 4) % 2) ? Color{255, 255, 255} : Color{32, 32, 32});
      }
    }
  } else if (name == "overlap") {
    // Near green square covers exactly the central half of a square viewport.
    // Submit far red last to catch a missing/reversed depth test.
    Quad(fixture.mesh, -1, -1, 1, 1, -2, -2, {0, 255, 0});
    Quad(fixture.mesh, -3, -3, 3, 3, -4, -4, {255, 0, 0});
  } else if (name == "clipped") {
    Quad(fixture.mesh, -3, -2, 3, 2, -0.25f, -4, {255, 180, 64});
    fixture.settings.cull_mode = CullMode::kNone;
  } else if (name == "micro") {
    constexpr int kCount = 96;
    for (int y = 0; y < kCount; ++y) {
      for (int x = 0; x < kCount; ++x) {
        const float left = -1.8f + 3.6f * x / kCount;
        const float bottom = -1.8f + 3.6f * y / kCount;
        Quad(fixture.mesh, left, bottom, left + 3.2f / kCount,
             bottom + 3.2f / kCount, -2, -2,
             {static_cast<uint8_t>(64 + x * 2),
              static_cast<uint8_t>(64 + y * 2), 180});
      }
    }
  } else if (name == "primitives") {
    fixture.primitives = true;
    fixture.camera = MakePrimitivesCamera();
    fixture.settings.ambient = 0.15f;
  } else if (name == "shrimp") {
    fixture.mesh = Mesh::LoadFromObj(RENDERER_ASSET_DIR "/Shrimp.obj");
    fixture.texture = LoadTexture(RENDERER_ASSET_DIR "/Shrimp_texture.png");
    if (fixture.mesh.triangles.empty() || fixture.texture.pixels.empty()) {
      throw std::runtime_error("Could not load benchmark shrimp assets");
    }
    fixture.camera.position = {0, 0, 3};
    fixture.camera.fov_y = Radians(60);
    fixture.camera.near = 0.1f;
    fixture.model = RotationY(Radians(65));
    fixture.settings.ambient = 0.15f;
  } else {
    throw std::invalid_argument("Unknown benchmark scene: " +
                                std::string(name));
  }
  if (!fixture.texture.pixels.empty() && fixture.texture.mipmaps.empty()) {
    fixture.texture.GenerateMipmaps();
  }
  return fixture;
}

Scene BenchmarkScene::Instantiate() const {
  if (primitives) {
    return MakePrimitivesScene();
  }
  auto geometry = std::make_shared<Mesh>(mesh);
  auto image =
      texture.pixels.empty() ? nullptr : std::make_shared<Texture>(texture);
  Scene result;
  result.objects.emplace_back(geometry);
  result.objects.back().texture = image;
  result.objects.back().model = model;
  return result;
}
