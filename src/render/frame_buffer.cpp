#include "math/math.h"
#include "frame_buffer.h"
#include "render/rasterer.h"

#include <algorithm>
#include <cmath>
#include <fstream>

void FrameBuffer::setPixel(int x, int y, Color color) {
  if (x < 0 || y < 0 || x >= static_cast<int>(width_) || y >= static_cast<int>(height_)) {
    return;
  }
  colors_[static_cast<size_t>(y) * width_ + x] = color;
}

void FrameBuffer::blendPixel(int x, int y, float depth, Color color) {
  if (x < 0 || y < 0 || x >= static_cast<int>(width_) || y >= static_cast<int>(height_)) {
    return;
  }
  size_t idx = static_cast<size_t>(y) * width_ + x;
  if (depth < depth_[idx]) {
    depth_[idx] = depth;
    colors_[idx] = color;
  }
}

void FrameBuffer::clear(Color color) {
  std::fill(colors_.begin(), colors_.end(), color);
  std::fill(depth_.begin(), depth_.end(), 1.0f);
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

  for (int y = y_min; y <= y_max; ++y) {
    for (int x = x_min; x <= x_max; ++x) {
      float px = x + 0.5f;
      float py = y + 0.5f;

      float w0 = signedArea(b.x, b.y, c.x, c.y, px, py) / area;
      float w1 = signedArea(c.x, c.y, a.x, a.y, px, py) / area;
      float w2 = signedArea(a.x, a.y, b.x, b.y, px, py) / area;

      if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
        float depth = w0 * a.depth + w1 * b.depth + w2 * c.depth;
        Color color = interpolateColor(a.color, b.color, c.color, w0, w1, w2);
        blendPixel(x, y, depth, color);
      }
    }
  }
}

bool FrameBuffer::savePPM(const std::string& filename) const {
  std::ofstream out(filename, std::ios::binary);
  if (!out) {
    return false;
  }

  out << "P6\n" << width_ << ' ' << height_ << "\n255\n";

  for (const Color& color : colors_) {
    out.put(static_cast<char>(color.r));
    out.put(static_cast<char>(color.g));
    out.put(static_cast<char>(color.b));
  }
  return out.good();
}