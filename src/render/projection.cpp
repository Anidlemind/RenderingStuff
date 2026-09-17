#include "render/projection.h"

#include "render/vertex.h"

bool Project(const Vertex3D& v, const Mat4& mvp, int width, int height,
             Vertex2D& out) {
  return ProjectFromClip(mvp * Vec4(v.x, v.y, v.z, 1.0f), v.uv, v.color, width,
                         height, out);
}

bool ProjectFromClip(const Vec4& clip, const Vec2& uv, Color color, int width,
                     int height, Vertex2D& out) {
  if (width <= 0 || height <= 0 || clip.w <= 0.0f || !std::isfinite(clip.x) ||
      !std::isfinite(clip.y) || !std::isfinite(clip.z) ||
      !std::isfinite(clip.w)) {
    return false;
  }

  const float inv_w = 1.0f / clip.w;
  const float ndc_x = clip.x * inv_w;
  const float ndc_y = clip.y * inv_w;
  const float ndc_z = clip.z * inv_w;

  out.x = (ndc_x + 1.0f) * 0.5f * width;
  out.y = (1.0f - ndc_y) * 0.5f * height;
  out.depth = ndc_z;
  out.inv_w = inv_w;
  out.uv = uv;
  out.color = {static_cast<float>(color.r), static_cast<float>(color.g),
               static_cast<float>(color.b)};
  return std::isfinite(out.x) && std::isfinite(out.y) &&
         std::isfinite(out.depth) && std::isfinite(out.inv_w);
}
