#include "render/clipping.h"

#include <algorithm>
#include <cmath>

#include "render/projection.h"

namespace {
double Distance(const Vec4& p, int plane) {
  switch (plane) {
    case 0:
      return static_cast<double>(p.w) + p.x;
    case 1:
      return static_cast<double>(p.w) - p.x;
    case 2:
      return static_cast<double>(p.w) + p.y;
    case 3:
      return static_cast<double>(p.w) - p.y;
    case 4:
      return static_cast<double>(p.w) + p.z;
    default:
      return static_cast<double>(p.w) - p.z;
  }
}

ClipVertex Intersect(const ClipVertex& inside, const ClipVertex& outside,
                     int plane) {
  const double di = Distance(inside.position, plane);
  const double t = di / (di - Distance(outside.position, plane));
  const auto mix = [t](float a, float b) {
    return static_cast<float>(static_cast<double>(a) +
                              t * (static_cast<double>(b) - a));
  };
  ClipVertex v{{mix(inside.position.x, outside.position.x),
                mix(inside.position.y, outside.position.y),
                mix(inside.position.z, outside.position.z),
                mix(inside.position.w, outside.position.w)},
               {mix(inside.uv.x, outside.uv.x), mix(inside.uv.y, outside.uv.y)},
               {mix(inside.color.x, outside.color.x),
                mix(inside.color.y, outside.color.y),
                mix(inside.color.z, outside.color.z)},
               {mix(inside.normal.x, outside.normal.x),
                mix(inside.normal.y, outside.normal.y),
                mix(inside.normal.z, outside.normal.z)},
               {mix(inside.world_position.x, outside.world_position.x),
                mix(inside.world_position.y, outside.world_position.y),
                mix(inside.world_position.z, outside.world_position.z)}};
  // Snap the intersection to its plane. Always computing inside -> outside
  // makes shared-edge intersections independent of triangle traversal order.
  switch (plane) {
    case 0:
      v.position.x = -v.position.w;
      break;
    case 1:
      v.position.x = v.position.w;
      break;
    case 2:
      v.position.y = -v.position.w;
      break;
    case 3:
      v.position.y = v.position.w;
      break;
    case 4:
      v.position.z = -v.position.w;
      break;
    default:
      v.position.z = v.position.w;
      break;
  }
  return v;
}

bool Finite(const ClipVertex& v) {
  return std::isfinite(v.position.x) && std::isfinite(v.position.y) &&
         std::isfinite(v.position.z) && std::isfinite(v.position.w) &&
         std::isfinite(v.uv.x) && std::isfinite(v.uv.y) &&
         std::isfinite(v.color.x) && std::isfinite(v.color.y) &&
         std::isfinite(v.color.z) && std::isfinite(v.normal.x) &&
         std::isfinite(v.normal.y) && std::isfinite(v.normal.z) &&
         std::isfinite(v.world_position.x) &&
         std::isfinite(v.world_position.y) && std::isfinite(v.world_position.z);
}
}  // namespace

ClippedPolygon ClipTriangle(const ClipVertex& a, const ClipVertex& b,
                            const ClipVertex& c) {
  if (!Finite(a) || !Finite(b) || !Finite(c)) {
    return {};
  }
  const auto outcode = [](const ClipVertex& vertex) {
    unsigned mask = 0;
    for (int plane = 0; plane < 6; ++plane) {
      if (Distance(vertex.position, plane) < 0) {
        mask |= 1u << plane;
      }
    }
    return mask;
  };
  const unsigned ma = outcode(a), mb = outcode(b), mc = outcode(c);
  if ((ma & mb & mc) != 0) {
    return {};
  }
  ClippedPolygon input;
  input.vertices[0] = a;
  input.vertices[1] = b;
  input.vertices[2] = c;
  input.size = 3;
  if ((ma | mb | mc) == 0) {
    return input;
  }
  for (int plane = 0; plane < 6 && input.size != 0; ++plane) {
    ClippedPolygon output;
    const auto same_position = [](const ClipVertex& lhs,
                                  const ClipVertex& rhs) {
      return lhs.position.x == rhs.position.x &&
             lhs.position.y == rhs.position.y &&
             lhs.position.z == rhs.position.z &&
             lhs.position.w == rhs.position.w;
    };
    const auto append = [&](const ClipVertex& v) {
      if (output.size == 0 ||
          !same_position(output.vertices[output.size - 1], v)) {
        output.vertices[output.size++] = v;
      }
    };
    auto previous = input.vertices[input.size - 1];
    bool previous_inside = Distance(previous.position, plane) >= 0;
    for (size_t i = 0; i < input.size; ++i) {
      const auto& current = input.vertices[i];
      const bool current_inside = Distance(current.position, plane) >= 0;
      if (current_inside != previous_inside) {
        append(current_inside ? Intersect(current, previous, plane)
                              : Intersect(previous, current, plane));
      }
      if (current_inside) {
        append(current);
      }
      previous = current;
      previous_inside = current_inside;
    }
    if (output.size > 1 &&
        same_position(output.vertices[0], output.vertices[output.size - 1])) {
      --output.size;
    }
    input = output;
  }
  return input;
}

bool ProjectClippedVertex(const ClipVertex& v, int width, int height,
                          Vertex2D& out) {
  if (!ProjectFromClip(v.position, v.uv, {}, width, height, out)) {
    return false;
  }
  out.color = v.color;
  out.normal = v.normal;
  out.world_position = v.world_position;
  // Clipping roundoff may leave a coordinate a few ulps past its boundary.
  out.x = std::clamp(out.x, 0.0f, static_cast<float>(width));
  out.y = std::clamp(out.y, 0.0f, static_cast<float>(height));
  out.depth = std::clamp(out.depth, -1.0f, 1.0f);
  return true;
}
