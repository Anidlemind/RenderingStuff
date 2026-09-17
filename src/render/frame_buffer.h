#ifndef RENDERER_SRC_RENDER_FRAME_BUFFER_H_
#define RENDERER_SRC_RENDER_FRAME_BUFFER_H_

#include <algorithm>
#include <string>
#include <vector>

#include "assets/texture.h"
#include "render/color.h"
#include "render/prepared_triangle.h"
#include "render/shading.h"
#include "render/vertex.h"

class FrameBuffer {
 private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  std::vector<uint32_t> colors_;
  std::vector<float> depth_;

 public:
  FrameBuffer(uint32_t w, uint32_t h);

  const uint32_t* Pixels() const { return colors_.data(); }
  const float* Depth() const { return depth_.data(); }
  void ResolveOverdraw();
  uint32_t Width() const { return width_; }
  uint32_t Height() const { return height_; }

  void SetPixel(int x, int y, Color color);

  void BlendPixel(int x, int y, float depth, Color color);

  void Clear(Color color);

  void ClearRange(Color color, int y_start, int y_end);
  // Resolve a 2x-sized source in linear RGB. Output is intended for
  // presentation.
  void ResolveSupersampling(const FrameBuffer& source);
  // Disjoint output row ranges can be resolved concurrently.
  void ResolveSupersamplingRange(const FrameBuffer& source, int y_start,
                                 int y_end);

  void DrawLine(int x0, int y0, int x1, int y1, Color color);

  void DrawTriangle(const Vertex2D& a, const Vertex2D& b, const Vertex2D& c,
                    const Texture* texture = nullptr);

  void DrawTriangleRange(const Vertex2D& a, const Vertex2D& b,
                         const Vertex2D& c, int y_start, int y_end,
                         const Texture* texture = nullptr);

  bool SavePpm(const std::string& filename) const;
  void DrawPreparedTriangle(const PreparedTriangle& triangle,
                            BoundingBox region,
                            const Texture* texture = nullptr,
                            const FragmentSettings* shading = nullptr);
};

#endif  // RENDERER_SRC_RENDER_FRAME_BUFFER_H_
