#include "assets/texture.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

#include "render/color_space.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Color Texture::Sample(float u, float v) const {
  if (width <= 0 || height <= 0 ||
      pixels.size() !=
          static_cast<size_t>(width) * static_cast<size_t>(height) ||
      !std::isfinite(u) || !std::isfinite(v)) {
    return {255, 255, 255};
  }

  u = u - std::floor(u);
  v = v - std::floor(v);

  int x = static_cast<int>(u * static_cast<float>(width));
  int y = static_cast<int>((1.0f - v) * static_cast<float>(height));

  x = std::clamp(x, 0, width - 1);
  y = std::clamp(y, 0, height - 1);

  return pixels[static_cast<size_t>(y) * width + x];
}

Texture LoadTexture(const std::string& path) {
  Texture tex;

  int w = 0, h = 0, channels = 0;
  const std::unique_ptr<unsigned char, decltype(&stbi_image_free)> data(
      stbi_load(path.c_str(), &w, &h, &channels, 3), stbi_image_free);

  if (!data) {
    std::fprintf(stderr, "loadTexture: failed to load '%s': %s\n", path.c_str(),
                 stbi_failure_reason());
    return tex;
  }

  tex.width = w;
  tex.height = h;
  tex.pixels.resize(static_cast<size_t>(w) * h);

  for (size_t i = 0; i < tex.pixels.size(); ++i) {
    tex.pixels[i].r = data.get()[i * 3 + 0];
    tex.pixels[i].g = data.get()[i * 3 + 1];
    tex.pixels[i].b = data.get()[i * 3 + 2];
  }

  tex.GenerateMipmaps();
  std::fprintf(stderr, "loadTexture: '%s' — %dx%d\n", path.c_str(), w, h);
  return tex;
}

void Texture::GenerateMipmaps() {
  mipmaps.clear();
  if (width <= 0 || height <= 0 ||
      pixels.size() != static_cast<size_t>(width) * height) {
    return;
  }
  MipLevel base{width, height, {}};
  base.pixels.reserve(pixels.size());
  for (const auto color : pixels) {
    base.pixels.push_back(ToLinear(color));
  }
  mipmaps.push_back(std::move(base));
  while (mipmaps.back().width > 1 || mipmaps.back().height > 1) {
    const auto& source = mipmaps.back();
    MipLevel next{
        std::max(1, source.width / 2), std::max(1, source.height / 2), {}};
    next.pixels.resize(static_cast<size_t>(next.width) * next.height);
    // Area-weighted box filtering includes every texel for odd dimensions.
    for (int y = 0; y < next.height; ++y) {
      const double top = static_cast<double>(y) * source.height / next.height;
      const double bottom =
          static_cast<double>(y + 1) * source.height / next.height;
      for (int x = 0; x < next.width; ++x) {
        const double left = static_cast<double>(x) * source.width / next.width;
        const double right =
            static_cast<double>(x + 1) * source.width / next.width;
        Vec3 sum{};
        for (int sy = static_cast<int>(top);
             sy < std::min(source.height, static_cast<int>(std::ceil(bottom)));
             ++sy) {
          for (int sx = static_cast<int>(left);
               sx < std::min(source.width, static_cast<int>(std::ceil(right)));
               ++sx) {
            const double weight = (std::min(right, sx + 1.0) -
                                   std::max(left, static_cast<double>(sx))) *
                                  (std::min(bottom, sy + 1.0) -
                                   std::max(top, static_cast<double>(sy)));
            sum += source.pixels[static_cast<size_t>(sy) * source.width + sx] *
                   static_cast<float>(weight);
          }
        }
        next.pixels[static_cast<size_t>(y) * next.width + x] =
            sum / static_cast<float>((right - left) * (bottom - top));
      }
    }
    mipmaps.push_back(std::move(next));
  }
}

Vec3 Texture::SampleLinear(float u, float v, TextureFilter filter,
                           float lod) const {
  if (width <= 0 || height <= 0 ||
      pixels.size() != static_cast<size_t>(width) * height ||
      !std::isfinite(u) || !std::isfinite(v)) {
    return {1, 1, 1};
  }
  u -= std::floor(u);
  v -= std::floor(v);
  const bool cached = !mipmaps.empty() && mipmaps.front().width == width &&
                      mipmaps.front().height == height;
  const auto level_sample = [&](size_t level, bool bilinear) {
    const MipLevel* mip = cached ? &mipmaps[level] : nullptr;
    const int w = mip ? mip->width : width, h = mip ? mip->height : height;
    const auto texel = [&](int x, int y) {
      x = (x % w + w) % w;
      y = (y % h + h) % h;
      const size_t index = static_cast<size_t>(y) * w + x;
      return mip ? mip->pixels[index] : ToLinear(pixels[index]);
    };
    const float x = u * w - 0.5f, y = (1 - v) * h - 0.5f;
    if (!bilinear) {
      return texel(static_cast<int>(std::floor(x + 0.5f)),
                   static_cast<int>(std::floor(y + 0.5f)));
    }
    const int ix = static_cast<int>(std::floor(x)),
              iy = static_cast<int>(std::floor(y));
    const float fx = x - ix, fy = y - iy;
    return (texel(ix, iy) * (1 - fx) + texel(ix + 1, iy) * fx) * (1 - fy) +
           (texel(ix, iy + 1) * (1 - fx) + texel(ix + 1, iy + 1) * fx) * fy;
  };
  if (filter != TextureFilter::kTrilinear || !cached) {
    return level_sample(0, filter != TextureFilter::kNearest);
  }
  if (std::isnan(lod)) {
    lod = 0;
  }
  lod = std::clamp(lod, 0.0f, static_cast<float>(mipmaps.size() - 1));
  const size_t low = static_cast<size_t>(std::floor(lod)),
               high = std::min(low + 1, mipmaps.size() - 1);
  const float fraction = lod - low;
  return level_sample(low, true) * (1 - fraction) +
         level_sample(high, true) * fraction;
}
