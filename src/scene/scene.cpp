#include "scene/scene.h"

#include <algorithm>
#include <stdexcept>

MeshInstance::MeshInstance(std::shared_ptr<const Mesh> geometry)
    : mesh_(std::move(geometry)) {
  if (!mesh_) {
    throw std::invalid_argument("Null mesh instance");
  }
  bounds_ = mesh_->Bounds();
}

namespace {
Vec4 Corner(const Bounds3D& bounds, int index) {
  return {index & 1 ? bounds.maximum.x : bounds.minimum.x,
          index & 2 ? bounds.maximum.y : bounds.minimum.y,
          index & 4 ? bounds.maximum.z : bounds.minimum.z, 1};
}
}  // namespace

bool IsVisible(const Bounds3D& bounds, const Mat4& clip_from_local) {
  unsigned common = 63;
  for (int i = 0; i < 8; ++i) {
    const auto p = clip_from_local * Corner(bounds, i);
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
        !std::isfinite(p.w)) {
      return true;
    }
    const unsigned code = (p.x < -p.w ? 1u : 0u) | (p.x > p.w ? 2u : 0u) |
                          (p.y < -p.w ? 4u : 0u) | (p.y > p.w ? 8u : 0u) |
                          (p.z < -p.w ? 16u : 0u) | (p.z > p.w ? 32u : 0u);
    common &= code;
  }
  return common == 0;
}

Bounds3D WorldBounds(const Scene& scene) {
  Bounds3D result{};
  bool first = true;
  for (const auto& object : scene.objects) {
    for (int i = 0; i < 8; ++i) {
      const auto p = object.model * Corner(object.Bounds(), i);
      if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
        throw std::invalid_argument("Invalid world bounds");
      }
      if (first) {
        result = {{p.x, p.y, p.z}, {p.x, p.y, p.z}};
        first = false;
      }
      result.minimum.x = std::min(result.minimum.x, p.x);
      result.minimum.y = std::min(result.minimum.y, p.y);
      result.minimum.z = std::min(result.minimum.z, p.z);
      result.maximum.x = std::max(result.maximum.x, p.x);
      result.maximum.y = std::max(result.maximum.y, p.y);
      result.maximum.z = std::max(result.maximum.z, p.z);
    }
  }
  return result;
}

Scene MakePrimitivesScene() {
  Scene scene;
  const auto add = [&](Mesh mesh, Vec3 position, float scale, float rotation,
                       Color color, float shininess) {
    for (auto& vertex : mesh.vertices) {
      vertex.color = color;
    }
    MeshInstance object(std::make_shared<Mesh>(std::move(mesh)));
    object.model =
        Translation(position) * RotationY(Radians(rotation)) * Scale(scale);
    object.material = Material{{255, 255, 255}, {80, 80, 80}, shininess};
    scene.objects.push_back(std::move(object));
  };
  add(Mesh::MakeCube(), {-2.2f, -0.35f, 0}, 0.75f, 25, {225, 115, 85}, 32);
  add(Mesh::MakeSphere(), {0, -0.15f, 0.25f}, 0.95f, 0, {75, 170, 185}, 96);
  add(Mesh::MakePyramid(), {2.2f, -0.15f, 0}, 0.95f, -15, {230, 185, 80}, 24);
  auto floor = std::make_shared<Mesh>();
  floor->vertices = {{-5, -1.1f, -3, {160, 170, 185}, {0, 1, 0}},
                     {-5, -1.1f, 3, {160, 170, 185}, {0, 1, 0}},
                     {5, -1.1f, 3, {160, 170, 185}, {0, 1, 0}},
                     {5, -1.1f, -3, {160, 170, 185}, {0, 1, 0}}};
  floor->triangles = {{0, 1, 2}, {0, 2, 3}};
  scene.objects.emplace_back(floor);
  scene.objects.back().casts_shadow = false;
  scene.objects.back().material = Material{{255, 255, 255}, {}, 1};
  return scene;
}

Camera MakePrimitivesCamera() {
  Camera camera;
  camera.position = {0, 2.8f, 7};
  camera.pitch = -0.4f;
  camera.fov_y = Radians(60);
  return camera;
}
