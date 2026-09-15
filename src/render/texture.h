#pragma once

#include <string>
#include <vector>

#include "render/color.h"

struct Texture {
  int width = 0;
  int height = 0;
  std::vector<Color> pixels;

  Color sample(float u, float v) const;
};

Texture loadTexture(const std::string& path);