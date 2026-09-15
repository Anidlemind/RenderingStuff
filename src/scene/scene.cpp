#include "scene/scene.h"
#include "render/obj_loader.h"
#include "math/math.h"

#include <algorithm>
#include <cmath>

Scene Scene::makeIcosahedron() {
  Scene s;

  const float phi = 1.618f;

  s.verts = {
    // (±1, ±φ, 0)
    {-1.0f,  phi,  0.0f, {255, 255, 255}},
    { 1.0f,  phi,  0.0f, {255, 255, 255}},
    {-1.0f, -phi,  0.0f, {255, 255, 255}},
    { 1.0f, -phi,  0.0f, {255, 255, 255}},

    // (0, ±1, ±φ)
    { 0.0f, -1.0f,  phi, {255, 255, 255}},
    { 0.0f,  1.0f,  phi, {255, 255, 255}},
    { 0.0f, -1.0f, -phi, {255, 255, 255}},
    { 0.0f,  1.0f, -phi, {255, 255, 255}},

    // (±φ, 0, ±1)
    { phi,  0.0f, -1.0f, {255, 255, 255}},
    { phi,  0.0f,  1.0f, {255, 255, 255}},
    {-phi,  0.0f, -1.0f, {255, 255, 255}},
    {-phi,  0.0f,  1.0f, {255, 255, 255}},
  };

 s.tris = {
    // 5 граней вокруг вершины 0
    { 0, 11,  5},
    { 0,  5,  1},
    { 0,  1,  7},
    { 0,  7, 10},
    { 0, 10, 11},

    // 5 граней вокруг вершины 3
    { 3,  9,  4},
    { 3,  4,  2},
    { 3,  2,  6},
    { 3,  6,  8},
    { 3,  8,  9},

    // оставшиеся 10 граней
    {11,  4,  5},
    { 5,  4,  9},
    { 5,  9,  1},
    { 1,  9,  8},
    { 1,  8,  7},
    { 7,  8,  6},
    { 7,  6, 10},
    {10,  6,  2},
    {10,  2, 11},
    {11,  2,  4},
  };  

  s.model = Mat4::identity();

  for (auto& v : s.verts) {
    v.normal = normalize(Vec3{v.x, v.y, v.z});
  }
  return s;
}

Scene Scene::loadFromOBJ(const std::string& path) {
  ObjMesh mesh = loadOBJ(path);

  if (mesh.verts.empty()) {
    std::fprintf(stderr, "Scene::loadFromOBJ: empty mesh from '%s'\n", path.c_str());
    return Scene{};
  }

  // Центрируем и масштабируем в диапазон примерно [-1, 1].
  Vec3 mn{ 1e30f,  1e30f,  1e30f};
  Vec3 mx{-1e30f, -1e30f, -1e30f};
  for (const auto& v : mesh.verts) {
    mn.x = std::min(mn.x, v.x); mn.y = std::min(mn.y, v.y); mn.z = std::min(mn.z, v.z);
    mx.x = std::max(mx.x, v.x); mx.y = std::max(mx.y, v.y); mx.z = std::max(mx.z, v.z);
  }

  const Vec3 center = (mn + mx) * 0.5f;
  const Vec3 size   = mx - mn;
  const float maxDim = std::max({size.x, size.y, size.z});
  const float scale  = (maxDim > 0.0f) ? (2.0f / maxDim) : 1.0f;

  for (auto& v : mesh.verts) {
    v.x = (v.x - center.x) * scale;
    v.y = (v.y - center.y) * scale;
    v.z = (v.z - center.z) * scale;
  }

  Scene s;
  s.verts = std::move(mesh.verts);
  s.tris  = std::move(mesh.tris);
  s.model = Mat4::identity();
  return s;
}