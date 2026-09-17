#include "scene/mesh.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <stdexcept>

#include "assets/obj_loader.h"
#include "math/math.h"

namespace {
void AddFace(Mesh& mesh, std::initializer_list<Vec3> corners) {
  const int offset = static_cast<int>(mesh.vertices.size());
  const auto* p = corners.begin();
  const auto normal = Normalize(Cross(p[1] - p[0], p[2] - p[0]));
  for (const auto& corner : corners) {
    mesh.vertices.push_back(
        {corner.x, corner.y, corner.z, {255, 255, 255}, normal});
  }
  for (int i = 2; i < static_cast<int>(corners.size()); ++i) {
    mesh.triangles.push_back({offset, offset + i - 1, offset + i});
  }
}
}  // namespace

Mesh Mesh::MakeCube() {
  Mesh mesh;
  AddFace(mesh, {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}});
  AddFace(mesh, {{1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}});
  AddFace(mesh, {{1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}});
  AddFace(mesh, {{-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}});
  AddFace(mesh, {{-1, 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}});
  AddFace(mesh, {{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}});
  return mesh;
}

Mesh Mesh::MakeSphere() {
  constexpr int kSegments = 32;
  constexpr int kRings = 16;
  Mesh mesh;
  for (int ring = 0; ring <= kRings; ++ring) {
    const float latitude = kPi * ring / kRings;
    for (int segment = 0; segment <= kSegments; ++segment) {
      const float longitude = 2 * kPi * segment / kSegments;
      const Vec3 position =
          ring == 0        ? Vec3{0, 1, 0}
          : ring == kRings ? Vec3{0, -1, 0}
                           : Vec3{std::sin(latitude) * std::cos(longitude),
                                  std::cos(latitude),
                                  std::sin(latitude) * std::sin(longitude)};
      mesh.vertices.push_back({position.x,
                               position.y,
                               position.z,
                               {255, 255, 255},
                               position,
                               {static_cast<float>(segment) / kSegments,
                                static_cast<float>(ring) / kRings}});
    }
  }
  for (int ring = 0; ring < kRings; ++ring) {
    for (int segment = 0; segment < kSegments; ++segment) {
      const int a = ring * (kSegments + 1) + segment;
      const int b = a + kSegments + 1;
      if (ring != 0) {
        mesh.triangles.push_back({a, a + 1, b});
      }
      if (ring != kRings - 1) {
        mesh.triangles.push_back({a + 1, b + 1, b});
      }
    }
  }
  return mesh;
}

Mesh Mesh::MakePyramid() {
  Mesh mesh;
  AddFace(mesh, {{-1, -1, 1}, {1, -1, 1}, {0, 1, 0}});
  AddFace(mesh, {{1, -1, 1}, {1, -1, -1}, {0, 1, 0}});
  AddFace(mesh, {{1, -1, -1}, {-1, -1, -1}, {0, 1, 0}});
  AddFace(mesh, {{-1, -1, -1}, {-1, -1, 1}, {0, 1, 0}});
  AddFace(mesh, {{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}});
  return mesh;
}

Bounds3D Mesh::Bounds() const {
  if (vertices.empty()) {
    return {};
  }
  const auto& first = vertices.front();
  Bounds3D bounds{{first.x, first.y, first.z}, {first.x, first.y, first.z}};
  for (const auto& v : vertices) {
    if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) {
      throw std::invalid_argument("Non-finite mesh position");
    }
    bounds.minimum.x = std::min(bounds.minimum.x, v.x);
    bounds.minimum.y = std::min(bounds.minimum.y, v.y);
    bounds.minimum.z = std::min(bounds.minimum.z, v.z);
    bounds.maximum.x = std::max(bounds.maximum.x, v.x);
    bounds.maximum.y = std::max(bounds.maximum.y, v.y);
    bounds.maximum.z = std::max(bounds.maximum.z, v.z);
  }
  return bounds;
}

Mesh Mesh::MakeIcosahedron() {
  Mesh s;

  const float phi = 1.618f;

  s.vertices = {
      // (±1, ±φ, 0)
      {-1.0f, phi, 0.0f, {255, 255, 255}},
      {1.0f, phi, 0.0f, {255, 255, 255}},
      {-1.0f, -phi, 0.0f, {255, 255, 255}},
      {1.0f, -phi, 0.0f, {255, 255, 255}},

      // (0, ±1, ±φ)
      {0.0f, -1.0f, phi, {255, 255, 255}},
      {0.0f, 1.0f, phi, {255, 255, 255}},
      {0.0f, -1.0f, -phi, {255, 255, 255}},
      {0.0f, 1.0f, -phi, {255, 255, 255}},

      // (±φ, 0, ±1)
      {phi, 0.0f, -1.0f, {255, 255, 255}},
      {phi, 0.0f, 1.0f, {255, 255, 255}},
      {-phi, 0.0f, -1.0f, {255, 255, 255}},
      {-phi, 0.0f, 1.0f, {255, 255, 255}},
  };

  s.triangles = {
      // 5 граней вокруг вершины 0
      {0, 11, 5},
      {0, 5, 1},
      {0, 1, 7},
      {0, 7, 10},
      {0, 10, 11},

      // 5 граней вокруг вершины 3
      {3, 9, 4},
      {3, 4, 2},
      {3, 2, 6},
      {3, 6, 8},
      {3, 8, 9},

      // оставшиеся 10 граней
      {11, 4, 5},
      {5, 4, 9},
      {5, 9, 1},
      {1, 9, 8},
      {1, 8, 7},
      {7, 8, 6},
      {7, 6, 10},
      {10, 6, 2},
      {10, 2, 11},
      {11, 2, 4},
  };

  for (auto& v : s.vertices) {
    v.normal = Normalize(Vec3{v.x, v.y, v.z});
  }
  return s;
}

Mesh Mesh::LoadFromObj(const std::string& path) {
  Mesh mesh = LoadObj(path);

  if (mesh.vertices.empty()) {
    return {};
  }

  // Центрируем и масштабируем в диапазон примерно [-1, 1].
  Vec3 mn{mesh.vertices[0].x, mesh.vertices[0].y, mesh.vertices[0].z};
  Vec3 mx = mn;
  for (const auto& v : mesh.vertices) {
    mn.x = std::min(mn.x, v.x);
    mn.y = std::min(mn.y, v.y);
    mn.z = std::min(mn.z, v.z);
    mx.x = std::max(mx.x, v.x);
    mx.y = std::max(mx.y, v.y);
    mx.z = std::max(mx.z, v.z);
  }

  const Vec3 center = (mn + mx) * 0.5f;
  const Vec3 size = mx - mn;
  const float max_dim = std::max({size.x, size.y, size.z});
  const float scale = (max_dim > 0.0f) ? (2.0f / max_dim) : 1.0f;

  for (auto& v : mesh.vertices) {
    if (std::isfinite(max_dim) && std::isfinite(scale) &&
        std::isfinite(center.x) && std::isfinite(center.y) &&
        std::isfinite(center.z)) {
      v.x = (v.x - center.x) * scale;
      v.y = (v.y - center.y) * scale;
      v.z = (v.z - center.z) * scale;
    } else {
      const double extent = std::max({static_cast<double>(mx.x) - mn.x,
                                      static_cast<double>(mx.y) - mn.y,
                                      static_cast<double>(mx.z) - mn.z});
      const auto normalize = [extent](float value, float low, float high) {
        return static_cast<float>(
            (value - (static_cast<double>(low) + high) * 0.5) *
            (extent > 0 ? 2 / extent : 1));
      };
      v.x = normalize(v.x, mn.x, mx.x);
      v.y = normalize(v.y, mn.y, mx.y);
      v.z = normalize(v.z, mn.z, mx.z);
    }
  }

  return mesh;
}
