#ifndef RENDERER_SRC_RENDER_VERTEX_H_
#define RENDERER_SRC_RENDER_VERTEX_H_

#include "math/math.h"
#include "render/color.h"

struct Vertex2D {
  float x = 0.0f;
  float y = 0.0f;
  float depth = 0.0f;

  // Legacy: RGB [0,255]. Modern shading: linear RGB [0,1].
  Vec3 color{};

  Vec2 uv{};

  float inv_w = 1.0f;
  Vec3 normal{};
  Vec3 world_position{};
};

struct Vertex3D {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  Color color{};

  Vec3 normal{};
  Vec2 uv{};
};

#endif  // RENDERER_SRC_RENDER_VERTEX_H_
