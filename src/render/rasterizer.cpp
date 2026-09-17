#include "render/rasterizer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
int BoundedInt(double value) {
  if (!std::isfinite(value)) {
    return 0;
  }
  return static_cast<int>(
      std::clamp(value, static_cast<double>(std::numeric_limits<int>::min()),
                 static_cast<double>(std::numeric_limits<int>::max())));
}
}  // namespace

BoundingBox ComputeBoundingBox(const Vertex2D& v0, const Vertex2D& v1,
                               const Vertex2D& v2) {
  return BoundingBox{
      .min_x = BoundedInt(std::floor(std::min({v0.x, v1.x, v2.x}))),
      .max_x = BoundedInt(std::ceil(std::max({v0.x, v1.x, v2.x}))),
      .min_y = BoundedInt(std::floor(std::min({v0.y, v1.y, v2.y}))),
      .max_y = BoundedInt(std::ceil(std::max({v0.y, v1.y, v2.y})))};
}
