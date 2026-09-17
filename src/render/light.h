#ifndef RENDERER_SRC_RENDER_LIGHT_H_
#define RENDERER_SRC_RENDER_LIGHT_H_

#include "math/math.h"
#include "render/color.h"

struct Light {
  Vec3 direction{0.5f, 1.0f, 0.5f};
  Color color{255, 255, 255};
  float intensity = 1.0f;
  float ambient = 0.15f;
};

struct Material {
  Color diffuse{255, 255, 255};
  Color specular{255, 255, 255};
  float shininess = 64.0f;
};

#endif  // RENDERER_SRC_RENDER_LIGHT_H_
