#pragma once

#include "color.h"
#include "vertex.h"
#include "texture.h"

#include <algorithm>
#include <vector>
#include <string>

class FrameBuffer {
private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  std::vector<uint32_t> colors_;
  std::vector<float> depth_;

public:
  FrameBuffer(uint32_t w, uint32_t h)
    : width_(w),
      height_(h),
      colors_(width_ * height_),
      depth_(width_ * height_, 1.0f) {}

  const uint32_t* pixels() const { return colors_.data(); }

  void setPixel(int x, int y, Color color);

  void blendPixel(int x, int y, float depth, Color color);

  void clear(Color color);

  void clearRange(Color color, int y_start, int y_end);

  void drawLine(int x0, int y0, int x1, int y1, Color color);

  void drawTriangle(const Vertex2D& a, const Vertex2D& b, const Vertex2D& c,
                    const Texture* texture = nullptr);

  void drawTriangleRange(const Vertex2D& a, const Vertex2D& b, const Vertex2D& c,
                         int y_start, int y_end,
                         const Texture* texture = nullptr);

  bool savePPM(const std::string& filename) const;
};
