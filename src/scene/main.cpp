#include "render/rasterer.h"
#include "render/vertex.h"
#include "render/color.h"
#include "math/math.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class FrameBuffer {
private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  std::vector<Color> pixels;

public:
  FrameBuffer(uint32_t w, uint32_t h) : width_(w), height_(h), pixels(width_ * height_) {}

  void setPixel(int x, int y, Color color) {
    if (x < 0 || y < 0 || x >= static_cast<int>(width_) || y >= static_cast<int>(height_)) {
      return;
    }
    pixels[y * width_ + x] = color;
  }

  void clear() {
    std::fill(pixels.begin(), pixels.end(), Color());
  }

  void drawLine(int x0, int y0, int x1, int y1, Color color) {
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

  void drawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    auto area = signedArea(v0, v1, v2);
    if (areEqual(area, 0.0f)) {
      return;
    }

    Vertex a = v0;
    Vertex b = v1;
    Vertex c = v2;
    if (area < 0.0f) {
      std::swap(a, b);
      area = -area;
    }

    auto bb= boundingBox(a, b, c);

    for (int y = std::max(bb.miny, 0); y <= std::min(bb.maxy, static_cast<int>(height_ - 1)); ++y) {
      for (int x = std::max(bb.minx, 0); x <= std::min(bb.maxx, static_cast<int>(width_ - 1)); ++x) {
        float px = x + 0.5f;
        float py = y + 0.5f;

        Vertex pv{px, py};

        float weight0 = signedArea(b, c, pv) / area;
        float weight1 = signedArea(c, a, pv) / area;
        float weight2 = signedArea(a, b, pv) / area;

        if (weight0 >= 0.0f && weight1 >= 0.0f && weight2 >= 0.0f) {
          setPixel(x, y, interpolateColor(a.color, b.color, c.color, weight0, weight1, weight2));
        }
      }
    }
  }

  bool savePPM(const std::string& filename) const {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
      return false;
    }

    out << "P6\n" << width_ << ' ' << height_ << "\n255\n";

    for (const Color& color : pixels) {
      out.put(static_cast<char>(color.r));
      out.put(static_cast<char>(color.g));
      out.put(static_cast<char>(color.b));
    }
    return out.good();
  }
};

int main() {
  constexpr uint32_t width = 800;
  constexpr uint32_t height = 600;

  FrameBuffer fb(width, height);
  
  // Grad
  for (uint32_t x = 0; x < width; ++x) {
    for (uint32_t y = 0; y < height; ++y) {
      uint8_t r = static_cast<uint8_t>(255 * x / (width - 1));
      uint8_t g = static_cast<uint8_t>(255 * y / (height - 1));
      uint8_t b = 128;

      Color Color{r, g, b};
      fb.setPixel(x, y, Color);
    }
  }

  // Lines
  fb.drawLine(0, height / 2, width - 1, height / 2, {0, 0, 0});
  fb.drawLine(width / 2, 0, width / 2, height - 1, {0, 0, 0});
  fb.drawLine(0, 0, width - 1, height - 1, {0, 0, 0});
  fb.drawLine(width - 1, 0, 0, height - 1, {0, 0, 0});

  // Triangle
  fb.drawTriangle(
    {400.0f, 100.0f, {255, 0, 0}},   // top, red
    {100.0f, 500.0f, {0, 255, 0}},   // bottom-left, green
    {700.0f, 500.0f, {0, 0, 255}}    // bottom-right, blue
);

  if (!fb.savePPM("./out.ppm")) {
    std::cerr << "Failed to save out.ppm\n";
  }

  std::cerr << "Saved out.ppm\n";
}