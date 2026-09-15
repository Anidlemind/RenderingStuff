#pragma once

#include <array>
#include <string>
#include <vector>

#include "render/vertex.h"

struct ObjMesh {
  std::vector<Vertex3D> verts;
  std::vector<std::array<int, 3>> tris;
};

ObjMesh loadOBJ(const std::string& path);