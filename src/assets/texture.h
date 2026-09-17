#ifndef RENDERER_SRC_ASSETS_TEXTURE_H_
#define RENDERER_SRC_ASSETS_TEXTURE_H_

#include <string>
#include <vector>

#include "math/math.h"
#include "render/color.h"

enum class TextureFilter { kNearest, kBilinear, kTrilinear };
struct MipLevel {
  int width = 0, height = 0;
  std::vector<Vec3> pixels;
};

struct Texture {
  int width = 0;
  int height = 0;
  std::vector<Color> pixels;
  // Linear RGB cache, including level zero. Rebuild after editing base pixels.
  std::vector<MipLevel> mipmaps{};

  Color Sample(float u, float v) const;
  void GenerateMipmaps();
  Vec3 SampleLinear(float u, float v, TextureFilter filter,
                    float lod = 0) const;
};

Texture LoadTexture(const std::string& path);

#endif  // RENDERER_SRC_ASSETS_TEXTURE_H_
