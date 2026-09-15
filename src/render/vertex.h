#pragma once

#include "color.h"

struct Vertex2D {
  float x = 0.0f;
  float y = 0.0f;
  float depth = 0.0f;
  Color color{};
};

struct Vertex3D {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  Color color{};
};
