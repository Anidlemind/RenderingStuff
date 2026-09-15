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

void FrameBuffer::drawTriangle(const Vertex2D& v0,
                               const Vertex2D& v1,
                               const Vertex2D& v2) {
  float area = signedArea(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
  if (areEqual(area, 0.0f)) {
    return;
  }

  Vertex2D a = v0;
  Vertex2D b = v1;
  Vertex2D c = v2;
  if (area < 0.0f) {
    std::swap(a, b);
    area = -area;
  }

  BoundingBox bb = boundingBox(a, b, c);

  int y_min = std::max(bb.miny, 0);
  int y_max = std::min(bb.maxy, static_cast<int>(height_) - 1);
  int x_min = std::max(bb.minx, 0);
  int x_max = std::min(bb.maxx, static_cast<int>(width_) - 1);

  const float dw0_dx = -(c.y - b.y) / area;
  const float dw0_dy =  (c.x - b.x) / area;
  const float dw1_dx = -(a.y - c.y) / area;
  const float dw1_dy =  (a.x - c.x) / area;
  const float dw2_dx = -(b.y - a.y) / area;
  const float dw2_dy =  (b.x - a.x) / area;

  const float px0 = x_min + 0.5f;
  const float py0 = y_min + 0.5f;

  float w0 = signedArea(b.x, b.y, c.x, c.y, px0, py0) / area;
  float w1 = signedArea(c.x, c.y, a.x, a.y, px0, py0) / area;
  float w2 = signedArea(a.x, a.y, b.x, b.y, px0, py0) / area;

  for (int y = y_min; y <= y_max; ++y) {
    float wr0 = w0;
    float wr1 = w1;
    float wr2 = w2;

    for (int x = x_min; x <= x_max; ++x) {
      if (wr0 >= 0.0f && wr1 >= 0.0f && wr2 >= 0.0f) {
        float depth = wr0 * a.depth + wr1 * b.depth + wr2 * c.depth;
        const size_t idx = static_cast<size_t>(y) * width_ + x;

        if (depth < depth_[idx]) {
          Color color = interpolateColor(a.color, b.color, c.color, wr0, wr1, wr2);
          depth_[idx] = depth;
          colors_[idx] = packColor(color);
        }
      }

      wr0 += dw0_dx;
      wr1 += dw1_dx;
      wr2 += dw2_dx;
    }

    w0 += dw0_dy;
    w1 += dw1_dy;
    w2 += dw2_dy;
  }
}

bool FrameBuffer::savePPM(const std::string& filename) const {
  std::ofstream out(filename, std::ios::binary);
  if (!out) {
    return false;
  }

  out << "P6\n" << width_ << ' ' << height_ << "\n255\n";

  for (uint32_t p : colors_) {
    const uint8_t r = static_cast<uint8_t>((p >> 16) & 0xFF);
    const uint8_t g = static_cast<uint8_t>((p >> 8)  & 0xFF);
    const uint8_t b = static_cast<uint8_t>( p        & 0xFF);

    out.put(static_cast<char>(r));
    out.put(static_cast<char>(g));
    out.put(static_cast<char>(b));
  }
  return out.good();
}