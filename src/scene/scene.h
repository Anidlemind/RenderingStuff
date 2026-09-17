#ifndef RENDERER_SRC_SCENE_SCENE_H_
#define RENDERER_SRC_SCENE_SCENE_H_

#include <optional>

#include "scene/camera.h"
#include "scene/mesh.h"

// Construct after geometry is complete; cached bounds belong to this snapshot.
// Do not mutate the shared mesh through another alias while using an instance.
class MeshInstance {
 public:
  explicit MeshInstance(std::shared_ptr<const Mesh> geometry);
  const Mesh& Geometry() const { return *mesh_; }
  const Bounds3D& Bounds() const { return bounds_; }
  Mat4 model = Mat4::Identity();
  std::optional<Material> material;
  std::shared_ptr<const Texture> texture;
  bool casts_shadow = true;
  bool receives_shadow = true;

 private:
  std::shared_ptr<const Mesh> mesh_;
  Bounds3D bounds_;
};

struct Scene {
  std::vector<MeshInstance> objects;
};

bool IsVisible(const Bounds3D& bounds, const Mat4& clip_from_local);
Bounds3D WorldBounds(const Scene& scene);
Scene MakePrimitivesScene();
Camera MakePrimitivesCamera();

#endif  // RENDERER_SRC_SCENE_SCENE_H_
