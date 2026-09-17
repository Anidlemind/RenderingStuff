#ifndef RENDERER_SRC_RENDER_PROJECTION_H_
#define RENDERER_SRC_RENDER_PROJECTION_H_

#include "math/math.h"
#include "render/vertex.h"

bool Project(const Vertex3D& v, const Mat4& mvp, int width, int height,
             Vertex2D& out);

bool ProjectFromClip(const Vec4& clip, const Vec2& uv, Color color, int width,
                     int height, Vertex2D& out);

#endif  // RENDERER_SRC_RENDER_PROJECTION_H_
