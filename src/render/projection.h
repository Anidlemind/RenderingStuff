#pragma once

#include "render/vertex.h"
#include "math/math.h"

bool project(const Vertex3D& v,
             const Mat4& mvp,
             int width,
             int height,
             Vertex2D& out);

bool projectFromClip(const Vec4& clip,
                     const Vec2& uv,
                     Color color,
                     int width,
                     int height,
                     Vertex2D& out);