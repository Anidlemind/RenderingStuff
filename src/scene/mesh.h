#ifndef RENDERER_SRC_SCENE_MESH_H_
#define RENDERER_SRC_SCENE_MESH_H_

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "assets/texture.h"
#include "render/light.h"
#include "render/vertex.h"

struct MeshMaterial {
  std::string name;
  Material surface{{255, 255, 255}, {64, 64, 64}, 64};
  std::shared_ptr<const Texture> texture;
};
struct Bounds3D {
  Vec3 minimum;
  Vec3 maximum;
};
// Geometry has no transform. Share it through const Mesh instances in a Scene.
struct Mesh {
  std::vector<Vertex3D> vertices;
  std::vector<std::array<int, 3>> triangles;
  std::vector<MeshMaterial> materials;
  // Empty means default material; otherwise one index per triangle
  // (-1=default).
  std::vector<int> triangle_materials;
  Bounds3D Bounds() const;
  static Mesh MakeCube();
  static Mesh MakeSphere();
  static Mesh MakePyramid();
  static Mesh MakeIcosahedron();
  static Mesh LoadFromObj(const std::string& path);
};

#endif  // RENDERER_SRC_SCENE_MESH_H_
