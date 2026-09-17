#ifndef RENDERER_SRC_RENDER_SOFTWARE_RENDERER_H_
#define RENDERER_SRC_RENDER_SOFTWARE_RENDERER_H_

#include <functional>
#include <memory>

#include "core/thread_pool.h"
#include "render/antialiasing.h"
#include "render/clipping.h"
#include "render/frame_buffer.h"
#include "render/shadow_map.h"
#include "scene/camera.h"
#include "scene/scene.h"

enum class CullMode { kNone, kBack, kFront };

struct RenderSettings {
  Color clear_color{20, 20, 30};
  Vec3 light_direction{0.5f, 1.0f, 0.5f};
  float ambient = 0.15f;
  CullMode cull_mode = CullMode::kBack;
  // 0 retains horizontal bands; 16/32 select square tiles.
  int tile_size = 0;
  // Core/benchmark default remains compatible; the viewer opts into Phong.
  ShadingMode shading = ShadingMode::kLegacy;
  TextureFilter filter = TextureFilter::kTrilinear;
  Material material{{255, 255, 255}, {64, 64, 64}, 64};
  Color light_color{255, 255, 255};
  float light_intensity = 1;
  Antialiasing antialiasing = Antialiasing::kNone;
  DebugView debug_view = DebugView::kNone;
  bool object_culling = true;
  bool textures = true;
  bool shadows = false;
  int shadow_size = 1024;
  float shadow_bias = 0.002f;
};

struct RenderStats {
  size_t input_triangles = 0;
  size_t raster_triangles = 0;
  float clear_ms = 0;
  float transform_ms = 0;
  float setup_ms = 0;
  float raster_ms = 0;
  float resolve_ms = 0;
  float shadow_ms = 0;
  size_t input_objects = 0;
  size_t culled_objects = 0;
};

// Synchronous render API. One instance must not be called concurrently.
// Scene, texture and framebuffer remain caller-owned.
class SoftwareRenderer {
 public:
  explicit SoftwareRenderer(int workers = 0);
  RenderStats Render(const Scene& scene, const Camera& camera,
                     const RenderSettings& settings, FrameBuffer& target,
                     const Texture* texture = nullptr);
  // Convenience for procedural geometry/tests; no ownership survives this call.
  RenderStats Render(const Mesh& mesh, const Camera& camera,
                     const RenderSettings& settings, FrameBuffer& target,
                     const Texture* texture = nullptr,
                     const Mat4& model = Mat4::Identity());
  int WorkerCount() const { return pool_.Size(); }

 private:
  struct WorkerScratch {
    std::vector<PreparedTriangle> triangles;
    std::vector<size_t> offsets;
    std::vector<size_t> cursors;
    std::vector<size_t> indices;
  };
  void RunWorkers(const std::function<void(int)>& task);
  RenderStats RenderScene(const Scene& scene, const Camera& camera,
                          const RenderSettings& settings, FrameBuffer& target,
                          const Texture* texture);
  RenderStats RenderMesh(const Mesh& mesh, const Mat4& model,
                         const Mat4& view_projection, const Camera& camera,
                         const RenderSettings& settings, FrameBuffer& target,
                         const Texture* texture, const Material* material,
                         const ShadowMap* shadow, bool depth_only = false);
  std::vector<ClipVertex> vertices_;
  std::vector<WorkerScratch> scratch_;
  std::unique_ptr<FrameBuffer> supersampled_;
  std::unique_ptr<FrameBuffer> shadow_target_;
  ShadowMap shadow_map_;
  ThreadPool pool_;
};

#endif  // RENDERER_SRC_RENDER_SOFTWARE_RENDERER_H_
