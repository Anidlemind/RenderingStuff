#include "math/math.h"
#include "frame_buffer.h"
#include "render/rasterer.h"

#include <algorithm>
#include <cmath>
#include <fstream>

inline uint32_t packColor(Color c) {
  return (static_cast<uint32_t>(c.r) << 16)
       | (static_cast<uint32_t>(c.g) << 8)
       |  static_cast<uint32_t>(c.b);
}

void FrameBuffer::setPixel(int x, int y, Color color) {
  if (x < 0 || y < 0 || x >= static_cast<int>(width_) || y >= static_cast<int>(height_)) {
    return;
  }
  colors_[static_cast<size_t>(y) * width_ + x] = packColor(color);
}

void FrameBuffer::blendPixel(int x, int y, float depth, Color color) {
  if (x < 0 || y < 0 || x >= static_cast<int>(width_) || y >= static_cast<int>(height_)) {
    return;
  }
  size_t idx = static_cast<size_t>(y) * width_ + x;
  if (depth < depth_[idx]) {
    depth_[idx] = depth;
    colors_[idx] = packColor(color);
  }
}

void FrameBuffer::clear(Color color) {
  const uint32_t packed = packColor(color);
  const size_t n = colors_.size();

  for (size_t i = 0; i < n; ++i) {
    colors_[i] = packed;
    depth_[i]  = 1.0f;
  }
}

void FrameBuffer::clearRange(Color color, int y_start, int y_end) {
  const uint32_t packed = packColor(color);
  const int ys = std::max(y_start, 0);
  const int ye = std::min(y_end, static_cast<int>(height_) - 1);
  if (ys > ye) return;

  const size_t begin = static_cast<size_t>(ys) * width_;
  const size_t end   = static_cast<size_t>(ye + 1) * width_;

  std::fill(colors_.begin() + begin, colors_.begin() + end, packed);
  std::fill(depth_.begin() + begin, depth_.begin() + end, 1.0f);
}

void FrameBuffer::drawLine(int x0, int y0, int x1, int y1, Color color) {
  int dx = std::abs(x1 - x0);
  int sign_x = (x1 >= x0) ? 1 : -1;

  int dy = std::abs(y1 - y0);
  int sign_y = (y1 >= y0) ? 1 : -1;

  int error = dx - dy;

  while (true) {
    setPixel(x0, y0, color);

    if (x0 == x1 && y0 == y1) {
      break;
    }
    int e2 = 2 * error;
    if (e2 > -dy) {
      error -= dy;
      x0 += sign_x;
    }
    if (e2 < dx) {
      error += dx;
      y0 += sign_y;
    }
  }
}

void FrameBuffer::drawTriangle(const Vertex2D& a, const Vertex2D& b, const Vertex2D& c,
                               const Texture* texture) {
  drawTriangleRange(a, b, c, 0, static_cast<int>(height_) - 1, texture);
}

void FrameBuffer::drawTriangleRange(const Vertex2D& v0,
                                    const Vertex2D& v1,
                                    const Vertex2D& v2,
                                    int y_start, int y_end,
                                    const Texture* texture) {
  float area = signedArea(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
  if (areEqual(area, 0.0f)) return;

  Vertex2D a = v0, b = v1, c = v2;
  if (area < 0.0f) {
    std::swap(a, b);
    area = -area;
  }

  BoundingBox bb = boundingBox(a, b, c);

  const int y_min = std::max(std::max(bb.miny, 0), y_start);
  const int y_max = std::min(std::min(bb.maxy, static_cast<int>(height_) - 1), y_end);
  const int x_min = std::max(bb.minx, 0);
  const int x_max = std::min(bb.maxx, static_cast<int>(width_) - 1);

  if (y_min > y_max || x_min > x_max) return;

  const float inv_area = 1.0f / area;

  for (int y = y_min; y <= y_max; ++y) {
    for (int x = x_min; x <= x_max; ++x) {
      const float px = x + 0.5f;
      const float py = y + 0.5f;

      const float E0 = signedArea(b.x, b.y, c.x, c.y, px, py);
      const float E1 = signedArea(c.x, c.y, a.x, a.y, px, py);
      const float E2 = signedArea(a.x, a.y, b.x, b.y, px, py);

      if (E0 < 0.0f || E1 < 0.0f || E2 < 0.0f) continue;

      const float w0 = E0 * inv_area;
      const float w1 = E1 * inv_area;
      const float w2 = E2 * inv_area;

      const float depth = w0 * a.depth + w1 * b.depth + w2 * c.depth;
      const size_t idx = static_cast<size_t>(y) * width_ + x;

      if (depth >= depth_[idx]) continue;

      Color color = interpolateColor(a.color, b.color, c.color, w0, w1, w2);

      if (texture) {
        // Perspective-correct UV.
        const float inv_w = w0 * a.inv_w + w1 * b.inv_w + w2 * c.inv_w;
        const float u = (w0 * a.uv.x * a.inv_w
                       + w1 * b.uv.x * b.inv_w
                       + w2 * c.uv.x * c.inv_w) / inv_w;
        const float v = (w0 * a.uv.y * a.inv_w
                       + w1 * b.uv.y * b.inv_w
                       + w2 * c.uv.y * c.inv_w) / inv_w;

        const Color texel = texture->sample(u, v);
        color.r = static_cast<uint8_t>((static_cast<int>(color.r) * texel.r) / 255);
        color.g = static_cast<uint8_t>((static_cast<int>(color.g) * texel.g) / 255);
        color.b = static_cast<uint8_t>((static_cast<int>(color.b) * texel.b) / 255);
      }

      depth_[idx] = depth;
      colors_[idx] = packColor(color);
    }
  }
}