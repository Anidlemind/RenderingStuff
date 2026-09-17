#ifndef RENDERER_SRC_RENDER_PREPARED_TRIANGLE_H_
#define RENDERER_SRC_RENDER_PREPARED_TRIANGLE_H_

#include <array>
#include <optional>

#include "render/rasterizer.h"

struct EdgeEquation {
  int64_t origin = 0;
  int64_t dx = 0;
  int64_t dy = 0;
  int64_t threshold = 0;
  int64_t At(int x, int y) const { return origin + dx * x + dy * y; }
};

// Immutable setup shared by every band/tile that touches this triangle.
// Construct using PrepareTriangle(); consumers require its validated
// invariants.
struct PreparedTriangle {
  Vertex2D a, b, c;
  BoundingBox bounds;
  std::array<EdgeEquation, 3> edges;
  double inverse_area = 0;
  std::optional<Color> constant_color;
  Vec2 uv_over_w_dx{}, uv_over_w_dy{};
  double reciprocal_w_dx = 0, reciprocal_w_dy = 0;
  int material_index = -1;
};

std::optional<PreparedTriangle> PrepareTriangle(Vertex2D a, Vertex2D b,
                                                Vertex2D c);

#endif  // RENDERER_SRC_RENDER_PREPARED_TRIANGLE_H_
