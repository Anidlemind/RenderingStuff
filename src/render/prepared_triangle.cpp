#include "render/prepared_triangle.h"

#include <algorithm>
#include <cmath>

std::optional<PreparedTriangle> PrepareTriangle(Vertex2D a, Vertex2D b,
                                                Vertex2D c) {
  const auto valid = [](const Vertex2D& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) &&
           std::abs(v.x) <= 1'000'000 && std::abs(v.y) <= 1'000'000 &&
           std::isfinite(v.depth) && std::isfinite(v.inv_w) && v.inv_w > 0 &&
           std::isfinite(v.color.x) && std::isfinite(v.color.y) &&
           std::isfinite(v.color.z);
  };
  if (!valid(a) || !valid(b) || !valid(c)) {
    return std::nullopt;
  }
  struct Point {
    int64_t x, y;
  };
  constexpr int64_t kScale = 256;
  const auto point = [](const Vertex2D& v) -> Point {
    return {std::llround(static_cast<double>(v.x) * kScale),
            std::llround(static_cast<double>(v.y) * kScale)};
  };
  const auto edge = [](Point from, Point to, Point p) {
    return (to.x - from.x) * (p.y - from.y) - (to.y - from.y) * (p.x - from.x);
  };
  auto pa = point(a), pb = point(b), pc = point(c);
  auto area = edge(pa, pb, pc);
  if (area == 0) {
    return std::nullopt;
  }
  if (area < 0) {
    std::swap(a, b);
    std::swap(pa, pb);
    area = -area;
  }
  const auto equation = [&](Point from, Point to) -> EdgeEquation {
    const bool inclusive = to.y < from.y || (to.y == from.y && to.x > from.x);
    return {edge(from, to, {kScale / 2, kScale / 2}), -(to.y - from.y) * kScale,
            (to.x - from.x) * kScale, inclusive ? 0 : 1};
  };
  PreparedTriangle result{
      a,
      b,
      c,
      ComputeBoundingBox(a, b, c),
      {equation(pb, pc), equation(pc, pa), equation(pa, pb)},
      1.0 / static_cast<double>(area),
      std::nullopt};
  // Equal integer colors remain identical under perspective interpolation.
  // Fractional colors retain the general path to preserve rounding at ties.
  const auto constant = [](float x, float y, float z) {
    return x == y && y == z && x >= 0 && x <= 255 && std::floor(x) == x;
  };
  if (constant(a.color.x, b.color.x, c.color.x) &&
      constant(a.color.y, b.color.y, c.color.y) &&
      constant(a.color.z, b.color.z, c.color.z)) {
    result.constant_color =
        Color{static_cast<uint8_t>(a.color.x), static_cast<uint8_t>(a.color.y),
              static_cast<uint8_t>(a.color.z)};
  }
  const Vertex2D vertices[]{a, b, c};
  for (int i = 0; i < 3; ++i) {
    const double dx =
        result.edges[i].dx * result.inverse_area * vertices[i].inv_w;
    const double dy =
        result.edges[i].dy * result.inverse_area * vertices[i].inv_w;
    result.reciprocal_w_dx += dx;
    result.reciprocal_w_dy += dy;
    result.uv_over_w_dx += vertices[i].uv * static_cast<float>(dx);
    result.uv_over_w_dy += vertices[i].uv * static_cast<float>(dy);
  }
  return result;
}
