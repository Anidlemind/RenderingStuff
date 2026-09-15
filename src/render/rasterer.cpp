#include "rasterer.h"

#include <algorithm>
#include <cmath>

BoundingBox boundingBox(const Vertex2D& v0, const Vertex2D& v1, const Vertex2D& v2){
  return BoundingBox{
    .minx = static_cast<int>(std::floor(std::min({v0.x, v1.x, v2.x}))),
    .maxx = static_cast<int>(std::ceil(std::max({v0.x, v1.x, v2.x}))),
    .miny = static_cast<int>(std::floor(std::min({v0.y, v1.y, v2.y}))),
    .maxy = static_cast<int>(std::ceil(std::max({v0.y, v1.y, v2.y})))
  };
}

Color interpolateColor(const Color& c0, const Color& c1, const Color& c2,
                       float w0, float w1, float w2) {
  auto mix = [](uint8_t a, uint8_t b, uint8_t c, float wa, float wb, float wc) {
    return static_cast<uint8_t>(wa * a + wb * b + wc * c + 0.5f);
  };
  return {
    mix(c0.r, c1.r, c2.r, w0, w1, w2),
    mix(c0.g, c1.g, c2.g, w0, w1, w2),
    mix(c0.b, c1.b, c2.b, w0, w1, w2)
  };
}

float interpolateDepth(const Vertex2D &a, const Vertex2D &b, const Vertex2D &c, float w0, float w1, float w2) {
  return a.depth * w0 + b.depth * w1 + c.depth * w2;
}