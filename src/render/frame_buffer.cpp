#include "render/frame_buffer.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>

#include "math/math.h"
#include "render/color_space.h"
#include "render/rasterizer.h"
#include "render/shadow_map.h"

FrameBuffer::FrameBuffer(uint32_t w, uint32_t h) : width_(w), height_(h) {
  if (w == 0 || h == 0 ||
      w > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
      h > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
      static_cast<size_t>(w) > std::numeric_limits<size_t>::max() / h) {
    throw std::invalid_argument("Invalid framebuffer dimensions");
  }
  const size_t count = static_cast<size_t>(w) * h;
  colors_.resize(count);
  depth_.assign(count, std::numeric_limits<float>::infinity());
}

bool FrameBuffer::SavePpm(const std::string& filename) const {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    return false;
  }
  file << "P6\n" << width_ << ' ' << height_ << "\n255\n";
  for (const uint32_t pixel : colors_) {
    const char rgb[] = {static_cast<char>((pixel >> 16) & 255),
                        static_cast<char>((pixel >> 8) & 255),
                        static_cast<char>(pixel & 255)};
    file.write(rgb, sizeof(rgb));
  }
  file.close();
  return !file.fail();
}

inline uint32_t PackColor(Color c) {
  return (static_cast<uint32_t>(c.r) << 16) |
         (static_cast<uint32_t>(c.g) << 8) | static_cast<uint32_t>(c.b);
}

void FrameBuffer::SetPixel(int x, int y, Color color) {
  if (x < 0 || y < 0 || x >= static_cast<int>(width_) ||
      y >= static_cast<int>(height_)) {
    return;
  }
  colors_[static_cast<size_t>(y) * width_ + x] = PackColor(color);
}

void FrameBuffer::BlendPixel(int x, int y, float depth, Color color) {
  if (x < 0 || y < 0 || x >= static_cast<int>(width_) ||
      y >= static_cast<int>(height_)) {
    return;
  }
  size_t idx = static_cast<size_t>(y) * width_ + x;
  if (std::isfinite(depth) && depth >= -1.0f && depth <= 1.0f &&
      depth < depth_[idx]) {
    depth_[idx] = depth;
    colors_[idx] = PackColor(color);
  }
}

void FrameBuffer::Clear(Color color) {
  const uint32_t packed = PackColor(color);
  const size_t n = colors_.size();

  for (size_t i = 0; i < n; ++i) {
    colors_[i] = packed;
    depth_[i] = std::numeric_limits<float>::infinity();
  }
}

void FrameBuffer::ClearRange(Color color, int y_start, int y_end) {
  const uint32_t packed = PackColor(color);
  const int ys = std::max(y_start, 0);
  const int ye = std::min(y_end, static_cast<int>(height_) - 1);
  if (ys > ye) {
    return;
  }

  const size_t begin = static_cast<size_t>(ys) * width_;
  const size_t end = static_cast<size_t>(ye + 1) * width_;

  std::fill(colors_.begin() + begin, colors_.begin() + end, packed);
  std::fill(depth_.begin() + begin, depth_.begin() + end,
            std::numeric_limits<float>::infinity());
}

void FrameBuffer::DrawLine(int x0, int y0, int x1, int y1, Color color) {
  // Clip before Bresenham to bound work and avoid overflowing int differences.
  const double start_x = x0, start_y = y0;
  const double delta_x = static_cast<double>(x1) - x0;
  const double delta_y = static_cast<double>(y1) - y0;
  double first = 0, last = 1;
  const auto clip = [&](double p, double q) {
    if (p == 0) {
      return q >= 0;
    }
    const double t = q / p;
    if (p < 0) {
      first = std::max(first, t);
    } else {
      last = std::min(last, t);
    }
    return first <= last;
  };
  if (!clip(-delta_x, start_x) || !clip(delta_x, width_ - 1.0 - start_x) ||
      !clip(-delta_y, start_y) || !clip(delta_y, height_ - 1.0 - start_y)) {
    return;
  }
  x0 = static_cast<int>(std::llround(start_x + first * delta_x));
  y0 = static_cast<int>(std::llround(start_y + first * delta_y));
  x1 = static_cast<int>(std::llround(start_x + last * delta_x));
  y1 = static_cast<int>(std::llround(start_y + last * delta_y));
  int64_t dx = std::abs(static_cast<int64_t>(x1) - x0);
  int sign_x = (x1 >= x0) ? 1 : -1;

  int64_t dy = std::abs(static_cast<int64_t>(y1) - y0);
  int sign_y = (y1 >= y0) ? 1 : -1;

  int64_t error = dx - dy;

  while (true) {
    SetPixel(x0, y0, color);

    if (x0 == x1 && y0 == y1) {
      break;
    }
    int64_t e2 = 2 * error;
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

void FrameBuffer::ResolveSupersampling(const FrameBuffer& source) {
  ResolveSupersamplingRange(source, 0, static_cast<int>(height_) - 1);
}

void FrameBuffer::ResolveSupersamplingRange(const FrameBuffer& source,
                                            int y_start, int y_end) {
  if (static_cast<uint64_t>(width_) * 2 != source.width_ ||
      static_cast<uint64_t>(height_) * 2 != source.height_) {
    throw std::invalid_argument(
        "SSAA source must be twice the destination size");
  }
  const auto linear = [](uint32_t pixel) {
    return ToLinear({static_cast<uint8_t>(pixel >> 16),
                     static_cast<uint8_t>(pixel >> 8),
                     static_cast<uint8_t>(pixel)});
  };
  const int first = std::max(y_start, 0);
  const int last = std::min(y_end, static_cast<int>(height_) - 1);
  if (first > last) {
    return;
  }
  for (int y = first; y <= last; ++y) {
    for (uint32_t x = 0; x < width_; ++x) {
      const size_t i = static_cast<size_t>(y) * 2 * source.width_ + x * 2;
      const auto a = source.colors_[i], b = source.colors_[i + 1],
                 c = source.colors_[i + source.width_],
                 d = source.colors_[i + source.width_ + 1];
      if (a == b && a == c && a == d) {
        colors_[static_cast<size_t>(y) * width_ + x] = a;
        continue;
      }
      const Vec3 average =
          (linear(a) + linear(b) + linear(c) + linear(d)) * 0.25f;
      colors_[static_cast<size_t>(y) * width_ + x] = PackColor(ToSrgb(average));
    }
  }
  // A resolved pixel has multiple depths. Do not expose a misleading sample.
  std::fill(depth_.begin() + static_cast<size_t>(first) * width_,
            depth_.begin() + static_cast<size_t>(last + 1) * width_,
            std::numeric_limits<float>::infinity());
}

void FrameBuffer::DrawTriangle(const Vertex2D& a, const Vertex2D& b,
                               const Vertex2D& c, const Texture* texture) {
  DrawTriangleRange(a, b, c, 0, static_cast<int>(height_) - 1, texture);
}

void FrameBuffer::DrawTriangleRange(const Vertex2D& v0, const Vertex2D& v1,
                                    const Vertex2D& v2, int y_start, int y_end,
                                    const Texture* texture) {
  if (const auto triangle = PrepareTriangle(v0, v1, v2)) {
    DrawPreparedTriangle(
        *triangle, {0, static_cast<int>(width_) - 1, y_start, y_end}, texture);
  }
}

void FrameBuffer::DrawPreparedTriangle(const PreparedTriangle& triangle,
                                       BoundingBox region,
                                       const Texture* texture,
                                       const FragmentSettings* shading) {
  const auto& a = triangle.a;
  const auto& b = triangle.b;
  const auto& c = triangle.c;
  const auto& bb = triangle.bounds;
  const int y_min = std::max({bb.min_y, 0, region.min_y});
  const int y_max =
      std::min({bb.max_y, static_cast<int>(height_) - 1, region.max_y});
  const int x_min = std::max({bb.min_x, 0, region.min_x});
  const int x_max =
      std::min({bb.max_x, static_cast<int>(width_) - 1, region.max_x});

  if (y_min > y_max || x_min > x_max) {
    return;
  }

  const double inv_area = triangle.inverse_area;
  const auto& edges = triangle.edges;
  bool full_coverage = true;
  // Affine edge extrema occur at rectangle corners. This works for tiles as
  // well as bands; the integer thresholds include the top-left edge rule.
  for (const auto& edge : edges) {
    const auto start = edge.At(x_min, y_min);
    const auto dx = edge.dx * (x_max - x_min);
    const auto dy = edge.dy * (y_max - y_min);
    if (start + std::max(int64_t{0}, dx) + std::max(int64_t{0}, dy) <
        edge.threshold) {
      return;
    }
    if (start + std::min(int64_t{0}, dx) + std::min(int64_t{0}, dy) <
        edge.threshold) {
      full_coverage = false;
    }
  }

  const auto lighting = shading && shading->mode != ShadingMode::kLegacy &&
                                shading->debug_view == DebugView::kNone &&
                                !shading->depth_only
                            ? PrepareLighting(*shading)
                            : PreparedLighting{};
  const auto shadow_depth_gradient =
      shading && shading->shadow_map && shading->mode == ShadingMode::kPhong &&
              shading->debug_view == DebugView::kNone && !shading->depth_only
          ? shading->shadow_map->ReceiverDepthGradient(
                a.world_position, b.world_position, c.world_position)
          : Vec2{};
  const auto same = [](Vec3 a, Vec3 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
  };
  // A uniformly colored matte face has constant direct/ambient lighting.
  // Only its shadow visibility depends on the fragment's world position.
  const bool flat_matte = shading && shading->mode == ShadingMode::kPhong &&
                          shading->debug_view == DebugView::kNone &&
                          !shading->depth_only && !texture &&
                          !lighting.has_specular && same(a.color, b.color) &&
                          same(a.color, c.color) && same(a.normal, b.normal) &&
                          same(a.normal, c.normal);
  Vec3 flat_ambient{}, flat_direct{};
  float flat_bias = 0;
  Color flat_color{};
  if (flat_matte) {
    const float diffuse =
        std::max(0.0f, Dot(Normalize(a.normal), lighting.direction));
    const auto base = Multiply(a.color, lighting.diffuse);
    flat_ambient = base * shading->light.ambient;
    flat_direct = Multiply(lighting.color,
                           base * ((1 - shading->light.ambient) * diffuse));
    flat_color = ToSrgb(flat_ambient + flat_direct * shading->light.intensity);
    if (shading->shadow_map) {
      flat_bias = shading->shadow_map->bias * (1 + 2 * (1 - diffuse));
    }
  }
  for (int y = y_min; y <= y_max; ++y) {
    int64_t e0 = edges[0].At(x_min, y), e1 = edges[1].At(x_min, y),
            e2 = edges[2].At(x_min, y);
    for (int x = x_min; x <= x_max;
         ++x, e0 += edges[0].dx, e1 += edges[1].dx, e2 += edges[2].dx) {
      if (!full_coverage &&
          (e0 < edges[0].threshold || e1 < edges[1].threshold ||
           e2 < edges[2].threshold)) {
        continue;
      }

      const double w0 = e0 * inv_area;
      const double w1 = e1 * inv_area;
      const double w2 = e2 * inv_area;

      const float depth = w0 * a.depth + w1 * b.depth + w2 * c.depth;
      const size_t idx = static_cast<size_t>(y) * width_ + x;

      if (!std::isfinite(depth) || depth < -1.0f || depth > 1.0f) {
        continue;
      }
      if (shading && shading->debug_view == DebugView::kOverdraw) {
        // Raw counts until the entire scene has finished. Count occluded
        // fragments too.
        if (colors_[idx] != std::numeric_limits<uint32_t>::max()) {
          ++colors_[idx];
        }
        continue;
      }
      if (depth >= depth_[idx]) {
        continue;
      }
      if (shading && shading->depth_only) {
        depth_[idx] = depth;
        continue;
      }
      if (shading && shading->debug_view == DebugView::kDepth) {
        const auto value = static_cast<uint8_t>(
            std::clamp((1 - depth) * 127.5f, 0.0f, 255.0f));
        depth_[idx] = depth;
        colors_[idx] = PackColor({value, value, value});
        continue;
      }
      if (shading && shading->debug_view == DebugView::kWireframe) {
        const auto distance = [](int64_t edge, const EdgeEquation& equation) {
          return static_cast<double>(edge) /
                 std::hypot(static_cast<double>(equation.dx),
                            static_cast<double>(equation.dy));
        };
        const bool border =
            std::min({distance(e0, edges[0]), distance(e1, edges[1]),
                      distance(e2, edges[2])}) <= 1;
        depth_[idx] = depth;
        colors_[idx] = border ? 0xffffff : 0;
        continue;
      }

      Color color;
      if (flat_matte && !shading->shadow_map) {
        depth_[idx] = depth;
        colors_[idx] = PackColor(flat_color);
        continue;
      }
      if (shading && shading->mode != ShadingMode::kLegacy) {
        const double reciprocal_w = w0 * a.inv_w + w1 * b.inv_w + w2 * c.inv_w;
        if (!std::isfinite(reciprocal_w) || reciprocal_w <= 0) {
          continue;
        }
        const double q0 = w0 * a.inv_w / reciprocal_w,
                     q1 = w1 * b.inv_w / reciprocal_w,
                     q2 = w2 * c.inv_w / reciprocal_w;
        const auto mix = [&](Vec3 va, Vec3 vb, Vec3 vc) -> Vec3 {
          return {static_cast<float>(q0 * va.x + q1 * vb.x + q2 * vc.x),
                  static_cast<float>(q0 * va.y + q1 * vb.y + q2 * vc.y),
                  static_cast<float>(q0 * va.z + q1 * vb.z + q2 * vc.z)};
        };
        if (flat_matte) {
          const float visibility = shading->shadow_map->VisibilityWithBias(
              mix(a.world_position, b.world_position, c.world_position),
              flat_bias, shadow_depth_gradient);
          color = visibility == 1
                      ? flat_color
                      : ToSrgb(flat_ambient +
                               flat_direct *
                                   (shading->light.intensity * visibility));
          depth_[idx] = depth;
          colors_[idx] = PackColor(color);
          continue;
        }
        if (shading->debug_view == DebugView::kNormals) {
          const auto n = Normalize(mix(a.normal, b.normal, c.normal));
          const auto channel = [](float value) {
            return static_cast<uint8_t>(
                std::clamp((value + 1) * 127.5f, 0.0f, 255.0f) + 0.5f);
          };
          depth_[idx] = depth;
          colors_[idx] = PackColor({channel(n.x), channel(n.y), channel(n.z)});
          continue;
        }
        Vec3 albedo = mix(a.color, b.color, c.color);
        if (texture) {
          const float u = q0 * a.uv.x + q1 * b.uv.x + q2 * c.uv.x;
          const float v = q0 * a.uv.y + q1 * b.uv.y + q2 * c.uv.y;
          float lod = 0;
          if (shading->filter == TextureFilter::kTrilinear) {
            const Vec2 dx{static_cast<float>((triangle.uv_over_w_dx.x -
                                              u * triangle.reciprocal_w_dx) /
                                             reciprocal_w),
                          static_cast<float>((triangle.uv_over_w_dx.y -
                                              v * triangle.reciprocal_w_dx) /
                                             reciprocal_w)};
            const Vec2 dy{static_cast<float>((triangle.uv_over_w_dy.x -
                                              u * triangle.reciprocal_w_dy) /
                                             reciprocal_w),
                          static_cast<float>((triangle.uv_over_w_dy.y -
                                              v * triangle.reciprocal_w_dy) /
                                             reciprocal_w)};
            lod = TextureLod(dx, dy, texture->width, texture->height);
          }
          albedo = Multiply(albedo,
                            texture->SampleLinear(u, v, shading->filter, lod));
        }
        const Vec3 linear =
            shading->mode == ShadingMode::kUnlit
                ? Multiply(albedo, lighting.diffuse)
                : ShadeBlinnPhong(
                      albedo, mix(a.normal, b.normal, c.normal),
                      mix(a.world_position, b.world_position, c.world_position),
                      *shading, lighting, shadow_depth_gradient);
        color = ToSrgb(linear);
      } else if (triangle.constant_color && !texture) {
        color = *triangle.constant_color;
      } else {
        const double inv_w = w0 * a.inv_w + w1 * b.inv_w + w2 * c.inv_w;
        if (!std::isfinite(inv_w) || inv_w <= 0.0) {
          continue;
        }
        const double q0 = w0 * a.inv_w / inv_w;
        const double q1 = w1 * b.inv_w / inv_w;
        const double q2 = w2 * c.inv_w / inv_w;
        const auto channel = [&](float ca, float cb, float cc) {
          return static_cast<uint8_t>(
              std::clamp(q0 * ca + q1 * cb + q2 * cc, 0.0, 255.0) + 0.5);
        };
        color = triangle.constant_color
                    ? *triangle.constant_color
                    : Color{channel(a.color.x, b.color.x, c.color.x),
                            channel(a.color.y, b.color.y, c.color.y),
                            channel(a.color.z, b.color.z, c.color.z)};

        if (texture) {
          // Perspective-correct UV.
          const float u = q0 * a.uv.x + q1 * b.uv.x + q2 * c.uv.x;
          const float v = q0 * a.uv.y + q1 * b.uv.y + q2 * c.uv.y;

          const Color texel = texture->Sample(u, v);
          color.r =
              static_cast<uint8_t>((static_cast<int>(color.r) * texel.r) / 255);
          color.g =
              static_cast<uint8_t>((static_cast<int>(color.g) * texel.g) / 255);
          color.b =
              static_cast<uint8_t>((static_cast<int>(color.b) * texel.b) / 255);
        }
      }

      depth_[idx] = depth;
      colors_[idx] = PackColor(color);
    }
  }
}

void FrameBuffer::ResolveOverdraw() {
  for (auto& pixel : colors_) {
    // Black=0, blue=1, green=2, yellow=3, red=4 or more fragments.
    constexpr uint32_t kPalette[] = {0, 0x3060ff, 0x40d060, 0xffd030, 0xff4030};
    pixel = kPalette[std::min(pixel, 4u)];
  }
}
