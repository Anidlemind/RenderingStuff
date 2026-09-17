#ifndef RENDERER_SRC_RENDER_RASTERIZER_H_
#define RENDERER_SRC_RENDER_RASTERIZER_H_

#include "render/bounding_box.h"
#include "render/color.h"
#include "render/vertex.h"

BoundingBox ComputeBoundingBox(const Vertex2D& v0, const Vertex2D& v1,
                               const Vertex2D& v2);

#endif  // RENDERER_SRC_RENDER_RASTERIZER_H_
