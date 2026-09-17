#ifndef RENDERER_SRC_RENDER_CLIPPING_H_
#define RENDERER_SRC_RENDER_CLIPPING_H_

#include <array>
#include <cstddef>

#include "render/vertex.h"

struct ClipVertex {
  Vec4 position;
  Vec2 uv;
  Vec3 color;
  Vec3 normal{};
  Vec3 world_position{};
};

// A triangle clipped by six planes has at most nine vertices. Extra capacity
// accommodates duplicate boundary vertices without heap allocations.
struct ClippedPolygon {
  std::array<ClipVertex, 12> vertices{};
  size_t size = 0;
};

// OpenGL-style homogeneous frustum: -w <= x,y,z <= w.
ClippedPolygon ClipTriangle(const ClipVertex& a, const ClipVertex& b,
                            const ClipVertex& c);
bool ProjectClippedVertex(const ClipVertex& vertex, int width, int height,
                          Vertex2D& out);

#endif  // RENDERER_SRC_RENDER_CLIPPING_H_
