#pragma once

#include <array>
#include <vector>

#include "math/math.h"
#include "render/vertex.h"

struct Scene {
  std::vector<Vertex3D> verts;
  std::vector<std::array<int, 3>> tris;
  Mat4 model = Mat4::identity();

  static Scene makeIcosahedron();
};