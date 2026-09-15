#include "render/projection.h"
#include "render/vertex.h"

bool project(const Vertex3D& v,
             const Mat4& mvp,
             int width,
             int height,
             Vertex2D& out) {
  Vec4 clip = mvp * Vec4(v.x, v.y, v.z, 1.0f);
  if (clip.w <= 0.0f) return false;

  const float inv_w = 1.0f / clip.w;
  const float ndc_x = clip.x * inv_w;
  const float ndc_y = clip.y * inv_w;
  const float ndc_z = clip.z * inv_w;

  out.x = (ndc_x + 1.0f) * 0.5f * width;
  out.y = (1.0f - ndc_y) * 0.5f * height;
  out.depth = ndc_z;
  out.inv_w = inv_w;
  out.uv = v.uv;
  out.color = v.color;
  return true;
}

bool projectFromClip(const Vec4& clip,
                     const Vec2& uv,
                     Color color,
                     int width,
                     int height,
                     Vertex2D& out) {
  if (clip.w <= 0.0f) return false;

  const float inv_w = 1.0f / clip.w;
  const float ndc_x = clip.x * inv_w;
  const float ndc_y = clip.y * inv_w;
  const float ndc_z = clip.z * inv_w;

  out.x = (ndc_x + 1.0f) * 0.5f * width;
  out.y = (1.0f - ndc_y) * 0.5f * height;
  out.depth = ndc_z;
  out.inv_w = inv_w;
  out.uv = uv;
  out.color = color;
  return true;
}