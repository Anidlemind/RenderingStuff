#include "texture.h"

#include <algorithm>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Color Texture::sample(float u, float v) const {
  if (width <= 0 || height <= 0) {
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

Texture loadTexture(const std::string& path) {
  Texture tex;

  int w = 0, h = 0, channels = 0;
  unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 3);

  if (!data) {
    std::fprintf(stderr, "loadTexture: failed to load '%s': %s\n",
                 path.c_str(), stbi_failure_reason());
    return tex;
  }

  tex.width = w;
  tex.height = h;
  tex.pixels.resize(static_cast<size_t>(w) * h);

  for (size_t i = 0; i < tex.pixels.size(); ++i) {
    tex.pixels[i].r = data[i * 3 + 0];
    tex.pixels[i].g = data[i * 3 + 1];
    tex.pixels[i].b = data[i * 3 + 2];
  }

  stbi_image_free(data);

  std::fprintf(stderr, "loadTexture: '%s' — %dx%d\n", path.c_str(), w, h);
  return tex;
}