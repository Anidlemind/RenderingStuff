#include "render/software_renderer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <stdexcept>

#include "render/color_space.h"
#include "render/rasterizer.h"

namespace {
int WorkerCount(int requested) {
  if (requested < 0 || requested > 256) {
    throw std::invalid_argument("Worker count must be in [0,256]");
  }
  return requested == 0 ? static_cast<int>(std::clamp(
                              std::thread::hardware_concurrency(), 1u, 256u))
                        : requested;
}
}  // namespace

SoftwareRenderer::SoftwareRenderer(int workers)
    : pool_(::WorkerCount(workers)) {
  scratch_.resize(pool_.Size());
}

void SoftwareRenderer::RunWorkers(const std::function<void(int)>& task) {
  // Drain even if submission fails: queued tasks capture this call's state.
  try {
    for (int worker = 0; worker < pool_.Size(); ++worker) {
      pool_.Submit([&, worker] { task(worker); });
    }
  } catch (...) {
    const auto error = std::current_exception();
    try {
      pool_.Wait();
    } catch (...) {
    }
    std::rethrow_exception(error);
  }
  pool_.Wait();
}

RenderStats SoftwareRenderer::Render(const Scene& scene, const Camera& camera,
                                     const RenderSettings& settings,
                                     FrameBuffer& target,
                                     const Texture* texture) {
  if (settings.antialiasing == Antialiasing::kNone) {
    return RenderScene(scene, camera, settings, target, texture);
  }
  if (settings.antialiasing != Antialiasing::kSsaa4) {
    throw std::invalid_argument("Unknown antialiasing mode");
  }
  if (target.Width() > 8192 || target.Height() > 8192) {
    throw std::invalid_argument("SSAA output dimensions exceed 8192");
  }
  if (!supersampled_ || supersampled_->Width() != target.Width() * 2 ||
      supersampled_->Height() != target.Height() * 2) {
    supersampled_ =
        std::make_unique<FrameBuffer>(target.Width() * 2, target.Height() * 2);
  }
  auto stats = RenderScene(scene, camera, settings, *supersampled_, texture);
  const auto start = std::chrono::steady_clock::now();
  const int height = static_cast<int>(target.Height());
  RunWorkers([&](int worker) {
    target.ResolveSupersamplingRange(*supersampled_,
                                     height * worker / pool_.Size(),
                                     height * (worker + 1) / pool_.Size() - 1);
  });
  stats.resolve_ms += std::chrono::duration<float, std::milli>(
                          std::chrono::steady_clock::now() - start)
                          .count();
  return stats;
}

Antialiasing ParseAntialiasing(std::string_view name) {
  if (name == "none") {
    return Antialiasing::kNone;
  }
  if (name == "ssaa4") {
    return Antialiasing::kSsaa4;
  }
  throw std::invalid_argument("Antialiasing must be none or ssaa4");
}

RenderStats SoftwareRenderer::Render(const Mesh& mesh, const Camera& camera,
                                     const RenderSettings& settings,
                                     FrameBuffer& target,
                                     const Texture* texture,
                                     const Mat4& model) {
  Scene scene;
  scene.objects.emplace_back(
      std::shared_ptr<const Mesh>(&mesh, [](const Mesh*) {}));
  scene.objects.back().model = model;
  return Render(scene, camera, settings, target, texture);
}

RenderStats SoftwareRenderer::RenderScene(const Scene& scene,
                                          const Camera& camera,
                                          const RenderSettings& settings,
                                          FrameBuffer& target,
                                          const Texture* texture) {
  using Clock = std::chrono::steady_clock;
  const auto start = Clock::now();
  const auto elapsed = [](auto since) {
    return std::chrono::duration<float, std::milli>(Clock::now() - since)
        .count();
  };
  if (target.Width() > 16384 || target.Height() > 16384 ||
      (settings.tile_size != 0 && settings.tile_size != 16 &&
       settings.tile_size != 32) ||
      !std::isfinite(camera.near) || !std::isfinite(camera.far) ||
      camera.near <= 0 || camera.far <= camera.near ||
      !std::isfinite(camera.fov_y) || camera.fov_y <= 0 ||
      camera.fov_y >= kPi || !std::isfinite(settings.ambient) ||
      settings.ambient < 0 || settings.ambient > 1 ||
      !std::isfinite(settings.shadow_bias) || settings.shadow_bias < 0 ||
      settings.shadow_size < 16 || settings.shadow_size > 4096) {
    throw std::invalid_argument("Invalid render settings");
  }
  for (float value : {camera.position.x, camera.position.y, camera.position.z,
                      camera.yaw, camera.pitch}) {
    if (!std::isfinite(value)) {
      throw std::invalid_argument("Invalid camera pose");
    }
  }
  const auto validate_material = [](const Material& material) {
    if (!std::isfinite(material.shininess) || material.shininess <= 0) {
      throw std::invalid_argument("Invalid material shininess");
    }
  };
  validate_material(settings.material);
  if (!std::isfinite(settings.light_intensity) ||
      settings.light_intensity < 0 ||
      !std::isfinite(settings.light_direction.x) ||
      !std::isfinite(settings.light_direction.y) ||
      !std::isfinite(settings.light_direction.z)) {
    throw std::invalid_argument("Invalid light");
  }
  RenderStats stats;
  stats.input_objects = scene.objects.size();
  for (const auto& object : scene.objects) {
    for (float value : object.model.m) {
      if (!std::isfinite(value)) {
        throw std::invalid_argument("Non-finite model transform");
      }
    }
    if (object.model.At(0, 3) != 0 || object.model.At(1, 3) != 0 ||
        object.model.At(2, 3) != 0 || object.model.At(3, 3) != 1) {
      throw std::invalid_argument("Model transform must be affine");
    }
    const auto& mesh = object.Geometry();
    stats.input_triangles += mesh.triangles.size();
    if (!mesh.triangle_materials.empty() &&
        mesh.triangle_materials.size() != mesh.triangles.size()) {
      throw std::invalid_argument("Triangle material count mismatch");
    }
    for (int index : mesh.triangle_materials) {
      if (index < -1 ||
          (index >= 0 && static_cast<size_t>(index) >= mesh.materials.size())) {
        throw std::invalid_argument("Invalid triangle material index");
      }
    }
    if (object.material) {
      validate_material(*object.material);
    }
    for (const auto& material : mesh.materials) {
      validate_material(material.surface);
    }
  }
  const int height = static_cast<int>(target.Height());
  RunWorkers([&](int worker) {
    target.ClearRange(settings.debug_view == DebugView::kNone
                          ? settings.clear_color
                          : Color{},
                      height * worker / pool_.Size(),
                      height * (worker + 1) / pool_.Size() - 1);
  });
  stats.clear_ms = elapsed(start);
  const ShadowMap* shadow = nullptr;
  if (settings.shadows && settings.shading == ShadingMode::kPhong &&
      settings.debug_view == DebugView::kNone && !scene.objects.empty()) {
    const auto shadow_start = Clock::now();
    if (!shadow_target_ || shadow_target_->Width() !=
                               static_cast<uint32_t>(settings.shadow_size)) {
      shadow_target_ = std::make_unique<FrameBuffer>(settings.shadow_size,
                                                     settings.shadow_size);
    }
    shadow_target_->Clear({});
    shadow_map_.size = settings.shadow_size;
    shadow_map_.bias = settings.shadow_bias;
    shadow_map_.view_projection =
        ShadowViewProjection(scene, settings.light_direction);
    auto depth_settings = settings;
    depth_settings.cull_mode = CullMode::kNone;
    for (const auto& object : scene.objects) {
      if (object.casts_shadow) {
        RenderMesh(object.Geometry(), object.model, shadow_map_.view_projection,
                   camera, depth_settings, *shadow_target_, nullptr, nullptr,
                   nullptr, true);
      }
    }
    shadow_map_.depth.assign(
        shadow_target_->Depth(),
        shadow_target_->Depth() +
            static_cast<size_t>(settings.shadow_size) * settings.shadow_size);
    shadow = &shadow_map_;
    stats.shadow_ms = elapsed(shadow_start);
  }
  const auto view_projection = camera.ViewProjection(
      static_cast<float>(target.Width()) / target.Height());
  for (const auto& object : scene.objects) {
    if (settings.object_culling &&
        !IsVisible(object.Bounds(), view_projection * object.model)) {
      ++stats.culled_objects;
      continue;
    }
    const auto result =
        RenderMesh(object.Geometry(), object.model, view_projection, camera,
                   settings, target, texture ? texture : object.texture.get(),
                   object.material ? &*object.material : nullptr,
                   object.receives_shadow ? shadow : nullptr);
    stats.raster_triangles += result.raster_triangles;
    stats.transform_ms += result.transform_ms;
    stats.setup_ms += result.setup_ms;
    stats.raster_ms += result.raster_ms;
  }
  if (settings.debug_view == DebugView::kOverdraw) {
    const auto resolve_start = Clock::now();
    target.ResolveOverdraw();
    stats.resolve_ms = elapsed(resolve_start);
  }
  return stats;
}

RenderStats SoftwareRenderer::RenderMesh(
    const Mesh& mesh, const Mat4& model, const Mat4& view_projection,
    const Camera& camera, const RenderSettings& settings, FrameBuffer& target,
    const Texture* texture, const Material* material, const ShadowMap* shadow,
    bool depth_only) {
  const int width = static_cast<int>(target.Width());
  const int height = static_cast<int>(target.Height());
  const bool modern = settings.shading != ShadingMode::kLegacy ||
                      settings.debug_view != DebugView::kNone;
  const auto normals =
      !depth_only && (settings.shading == ShadingMode::kPhong ||
                      settings.debug_view == DebugView::kNormals)
          ? NormalMatrix(model)
          : Mat4::Identity();
  FragmentSettings shading;
  shading.mode = settings.shading;
  if (settings.debug_view != DebugView::kNone) {
    shading.mode = ShadingMode::kUnlit;
  }
  shading.debug_view = settings.debug_view;
  shading.depth_only = depth_only;
  shading.shadow_map = shadow;
  shading.filter = settings.filter;
  shading.material = settings.material;
  if (material) {
    shading.material = *material;
  }
  shading.camera_position = camera.position;
  shading.light.direction = settings.light_direction;
  shading.light.color = settings.light_color;
  shading.light.ambient = settings.ambient;
  shading.light.intensity = settings.light_intensity;

  using Clock = std::chrono::steady_clock;
  auto elapsed = [](auto a, auto b) {
    return std::chrono::duration<float, std::milli>(b - a).count();
  };
  RenderStats stats;
  stats.input_triangles = mesh.triangles.size();
  const int workers = pool_.Size();
  const int band_count = std::min(height, workers * 8);
  const int band_height = (height + band_count - 1) / band_count;
  const int cell_width = settings.tile_size == 0 ? width : settings.tile_size;
  const int cell_height =
      settings.tile_size == 0 ? band_height : settings.tile_size;
  const int columns = (width + cell_width - 1) / cell_width;
  const int rows = (height + cell_height - 1) / cell_height;
  const int cell_count = columns * rows;
  const auto bins = [&](const PreparedTriangle& triangle, const auto& visit) {
    const auto& bounds = triangle.bounds;
    const int left = std::clamp(bounds.min_x / cell_width, 0, columns - 1);
    const int right = std::clamp(bounds.max_x / cell_width, 0, columns - 1);
    const int top = std::clamp(bounds.min_y / cell_height, 0, rows - 1);
    const int bottom = std::clamp(bounds.max_y / cell_height, 0, rows - 1);
    for (int y = top; y <= bottom; ++y) {
      for (int x = left; x <= right; ++x) {
        visit(y * columns + x);
      }
    }
  };
  auto cleared = Clock::now();
  vertices_.resize(mesh.vertices.size());
  const auto mvp = view_projection * model;
  const auto light = Normalize(settings.light_direction);
  RunWorkers([&](int worker) {
    const size_t begin = mesh.vertices.size() * worker / workers;
    const size_t end = mesh.vertices.size() * (worker + 1) / workers;
    for (size_t i = begin; i < end; ++i) {
      const auto& v = mesh.vertices[i];
      vertices_[i] = {mvp * Vec4{v.x, v.y, v.z, 1}, v.uv, {}};
      if (depth_only) {
        continue;
      }
      if (modern) {
        const auto world = model * Vec4{v.x, v.y, v.z, 1};
        vertices_[i].color = ToLinear(v.color);
        vertices_[i].normal = Normalize(TransformDirection(normals, v.normal));
        vertices_[i].world_position = {world.x, world.y, world.z};
      } else {
        const auto normal = Normalize(TransformDirection(model, v.normal));
        const float intensity =
            settings.ambient +
            (1 - settings.ambient) * std::max(0.0f, Dot(normal, light));
        vertices_[i].color = {v.color.r * intensity, v.color.g * intensity,
                              v.color.b * intensity};
      }
    }
  });
  auto transformed = Clock::now();
  RunWorkers([&](int worker) {
    auto& local = scratch_[worker];
    local.triangles.clear();
    local.offsets.assign(cell_count + 1, 0);
    const size_t begin = mesh.triangles.size() * worker / workers;
    const size_t end = mesh.triangles.size() * (worker + 1) / workers;
    for (size_t i = begin; i < end; ++i) {
      const auto& indices = mesh.triangles[i];
      for (int index : indices) {
        if (index < 0 || static_cast<size_t>(index) >= vertices_.size()) {
          throw std::invalid_argument("Invalid scene triangle index");
        }
      }
      const auto polygon = ClipTriangle(
          vertices_[indices[0]], vertices_[indices[1]], vertices_[indices[2]]);
      for (size_t j = 2; j < polygon.size; ++j) {
        struct {
          Vertex2D a, b, c;
        } triangle;
        if (!ProjectClippedVertex(polygon.vertices[0], width, height,
                                  triangle.a) ||
            !ProjectClippedVertex(polygon.vertices[j - 1], width, height,
                                  triangle.b) ||
            !ProjectClippedVertex(polygon.vertices[j], width, height,
                                  triangle.c)) {
          continue;
        }
        const float area = SignedArea(triangle.a.x, triangle.a.y, triangle.b.x,
                                      triangle.b.y, triangle.c.x, triangle.c.y);
        // CCW in NDC is negative area after the viewport's Y flip.
        if (area == 0 || (settings.cull_mode == CullMode::kBack && area >= 0) ||
            (settings.cull_mode == CullMode::kFront && area <= 0)) {
          continue;
        }
        if (auto prepared =
                PrepareTriangle(triangle.a, triangle.b, triangle.c)) {
          if (!mesh.triangle_materials.empty()) {
            prepared->material_index = mesh.triangle_materials[i];
          }
          local.triangles.push_back(*prepared);
          bins(*prepared, [&](int cell) { ++local.offsets[cell + 1]; });
        }
      }
    }
    // Count / prefix-sum / scatter yields contiguous bins, reusing capacity.
    for (int cell = 0; cell < cell_count; ++cell) {
      local.offsets[cell + 1] += local.offsets[cell];
    }
    local.indices.resize(local.offsets.back());
    local.cursors = local.offsets;
    for (size_t i = 0; i < local.triangles.size(); ++i) {
      bins(local.triangles[i],
           [&](int cell) { local.indices[local.cursors[cell]++] = i; });
    }
  });
  for (const auto& local : scratch_) {
    stats.raster_triangles += local.triangles.size();
  }
  auto setup = Clock::now();
  std::atomic<int> next_cell{0};
  RunWorkers([&](int) {
    for (;;) {
      const int cell = next_cell.fetch_add(1, std::memory_order_relaxed);
      if (cell >= cell_count) {
        break;
      }
      const int left = (cell % columns) * cell_width;
      const int top = (cell / columns) * cell_height;
      const BoundingBox region{left, std::min(left + cell_width, width) - 1,
                               top, std::min(top + cell_height, height) - 1};
      // Worker partitions preserve scene order, including equal-depth ties.
      for (const auto& local : scratch_) {
        for (size_t i = local.offsets[cell]; i < local.offsets[cell + 1]; ++i) {
          const auto& triangle = local.triangles[local.indices[i]];
          auto fragment = shading;
          const Texture* surface_texture = texture;
          if (!depth_only && triangle.material_index >= 0) {
            const auto& surface = mesh.materials[triangle.material_index];
            if (!material) {
              fragment.material = surface.surface;
            }
            if (!surface_texture) {
              surface_texture = surface.texture.get();
            }
          }
          target.DrawPreparedTriangle(
              triangle, region,
              settings.textures && !depth_only ? surface_texture : nullptr,
              modern || depth_only ? &fragment : nullptr);
        }
      }
    }
  });
  stats.transform_ms = elapsed(cleared, transformed);
  stats.setup_ms = elapsed(transformed, setup);
  stats.raster_ms = elapsed(setup, Clock::now());
  return stats;
}
