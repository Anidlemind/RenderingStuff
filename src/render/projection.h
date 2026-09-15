#pragma once

#include "vertex.h"
#include "math/math.h"

bool project(const Vertex3D& v,
             const Mat4& mvp,
             int width,
             int height,
             Vertex2D& out);
