#pragma once

#include "color.h"
#include "math/math.h"

struct Vertex2D {
  float x = 0.0f;
  float y = 0.0f;
  float depth = 0.0f;
  
  Color color{};
  
  Vec2  uv{};
  
  float inv_w = 1.0f;
};

struct Vertex3D {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  Color color{};

  Vec3 normal{};
  Vec2  uv{};
};
