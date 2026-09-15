#include "app/app.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

#include "math/math.h"
#include "render/projection.h"
#include "render/text.h"

namespace {
  constexpr int kWidth  = 1920;
  constexpr int kHeight = 1080;

  constexpr Vec3 kLightDirRaw{0.5f, 1.0f, 0.5f};
  constexpr float kAmbient = 0.15f;
}

App::App()
  : window_("renderer", kWidth, kHeight),
    framebuffer_(static_cast<uint32_t>(kWidth), static_cast<uint32_t>(kHeight)),
    camera_(),
    cameraController_(),
    scene_(Scene::loadFromOBJ("./assets/Shrimp.obj")),
    pool_(std::max(1u, std::thread::hardware_concurrency())),
    texture_(loadTexture("./assets/Shrimp_texture.png")) {
  camera_.position = {0.0f, 0.0f, 5.0f};
  camera_.yaw      = 0.0f;
  camera_.pitch    = 0.0f;
  camera_.fovY     = radians(60.0f);
  camera_.near     = 0.1f;
  camera_.far      = 100.0f;
}

App::~App() = default;

int App::run() {
  using clock = std::chrono::steady_clock;

  auto last = clock::now();

  while (window_.isOpen()) {
    window_.pollEvents();

    const auto now = clock::now();
    const float dt = std::chrono::duration<float>(now - last).count();
    last = now;

    if (dt > 1e-6f) {
      const float instant = 1.0f / dt;
      const float alpha = std::min(1.0f, dt * 5.0f);
      if (smoothedFps_ <= 0.0f) {
        smoothedFps_ = instant;
      } else {
        smoothedFps_ = smoothedFps_ * (1.0f - alpha) + instant * alpha;
      }
    }

    update(std::min(dt, 0.1f));
    render();
  }

  return 0;
}

void App::update(float dt) {
  cameraController_.update(camera_, window_.input(), dt);
}

void App::render() {
  using clk = std::chrono::steady_clock;
  auto ms = [](auto a, auto b) {
    return std::chrono::duration<float, std::milli>(b - a).count();
  };

  const int numThreads = pool_.size();

  // Clear
  auto t0 = clk::now();
  {
    const int rowsPerThread = (kHeight + numThreads - 1) / numThreads;
    for (int t = 0; t < numThreads; ++t) {
      const int ys = t * rowsPerThread;
      const int ye = std::min((t + 1) * rowsPerThread, kHeight) - 1;
      if (ys > ye) continue;

      pool_.submit([this, ys, ye]() {
        framebuffer_.clearRange({20, 20, 30}, ys, ye);
      });
    }
    pool_.wait();
  }
  auto t1 = clk::now();

  const float aspect = static_cast<float>(kWidth) / static_cast<float>(kHeight);
  const Mat4 mvp = camera_.viewProjection(aspect) * scene_.model;
  const Vec3 lightDir = normalize(kLightDirRaw);

  const size_t n = scene_.verts.size();

  static std::vector<Vec4> clipPositions;
  static std::vector<Color> litColors;
  if (clipPositions.size() < n) {
    clipPositions.resize(n);
    litColors.resize(n);
  }

  // Precompute
  {
    const size_t chunk = (n + numThreads - 1) / numThreads;
    for (int t = 0; t < numThreads; ++t) {
      const size_t s = t * chunk;
      const size_t e = std::min(s + chunk, n);
      if (s >= e) continue;

      pool_.submit([&, s, e]() {
        for (size_t i = s; i < e; ++i) {
          const Vertex3D& v = scene_.verts[i];

          clipPositions[i] = mvp * Vec4{v.x, v.y, v.z, 1.0f};

          const Vec3 N = normalize(transformDirection(scene_.model, v.normal));
          const float diff = std::max(0.0f, dot(N, lightDir));
          const float intensity = kAmbient + (1.0f - kAmbient) * diff;

          auto channel = [intensity](uint8_t c) {
            const float x = static_cast<float>(c) * intensity;
            return static_cast<uint8_t>(std::min(255.0f, x));
          };

          litColors[i] = {channel(v.color.r), channel(v.color.g), channel(v.color.b)};
        }
      });
    }
    pool_.wait();
  }
  auto t2 = clk::now();

  struct ProjectedTri { Vertex2D a, b, c; };

  static std::vector<ProjectedTri> projectedTris;
  projectedTris.resize(scene_.tris.size());

  const int numBands = numThreads * 8;
  const int bandHeight = (kHeight + numBands - 1) / numBands;

  static std::vector<std::vector<std::vector<int>>> localBands;
  if (localBands.size() != static_cast<size_t>(numThreads)) {
    localBands.resize(numThreads);
  }
  for (auto& perThread : localBands) {
    if (perThread.size() != static_cast<size_t>(numBands)) {
      perThread.resize(numBands);
    }
    for (auto& b : perThread) b.clear();
  }

  // Binning
  {
    const size_t triChunk = (scene_.tris.size() + numThreads - 1) / numThreads;
    for (int t = 0; t < numThreads; ++t) {
      const size_t s = t * triChunk;
      const size_t e = std::min(s + triChunk, scene_.tris.size());
      if (s >= e) continue;

      pool_.submit([&, t, s, e]() {
        auto& myBands = localBands[t];
        for (size_t i = s; i < e; ++i) {
          const auto& tri = scene_.tris[i];
          ProjectedTri& pt = projectedTris[i];

          if (!projectFromClip(clipPositions[tri[0]], scene_.verts[tri[0]].uv, litColors[tri[0]], kWidth, kHeight, pt.a)) continue;
          if (!projectFromClip(clipPositions[tri[1]], scene_.verts[tri[1]].uv, litColors[tri[1]], kWidth, kHeight, pt.b)) continue;
          if (!projectFromClip(clipPositions[tri[2]], scene_.verts[tri[2]].uv, litColors[tri[2]], kWidth, kHeight, pt.c)) continue;

          if (signedArea(pt.a.x, pt.a.y, pt.b.x, pt.b.y, pt.c.x, pt.c.y) >= 0.0f) continue;

          const int ymin = static_cast<int>(std::floor(std::min({pt.a.y, pt.b.y, pt.c.y})));
          const int ymax = static_cast<int>(std::ceil (std::max({pt.a.y, pt.b.y, pt.c.y})));

          const int firstBand = std::max(0, ymin / bandHeight);
          const int lastBand  = std::min(numBands - 1, ymax / bandHeight);

          for (int b = firstBand; b <= lastBand; ++b) {
            myBands[b].push_back(static_cast<int>(i));
          }
        }
      });
    }
    pool_.wait();
  }
  auto t3 = clk::now();

  // Render
  std::atomic<int> nextBand{0};
  for (int t = 0; t < numThreads; ++t) {
    pool_.submit([&]() {
      while (true) {
        const int b = nextBand.fetch_add(1, std::memory_order_relaxed);
        if (b >= numBands) break;

        const int y_start = b * bandHeight;
        const int y_end   = std::min((b + 1) * bandHeight, kHeight) - 1;

        for (int tt = 0; tt < numThreads; ++tt) {
          for (int idx : localBands[tt][b]) {
            const ProjectedTri& pt = projectedTris[idx];
            framebuffer_.drawTriangleRange(pt.a, pt.b, pt.c, y_start, y_end, &texture_);
          }
        }
      }
    });
  }
  pool_.wait();
  auto t4 = clk::now();

  {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "FPS: %d",
                  static_cast<int>(smoothedFps_ + 0.5f));
    drawText(framebuffer_, 11, 11, buf, {0, 0, 0});
    drawText(framebuffer_, 10, 10, buf, {255, 255, 255});
  }

  window_.present(framebuffer_.pixels(), kWidth, kHeight);
  auto t5 = clk::now();

  static int frame = 0;
  if (++frame % 30 == 0) {
    std::fprintf(stderr,
      "clear %.1f  precompute %.1f  binning %.1f  render %.1f  present %.1f  total %.1f ms\n",
      ms(t0, t1), ms(t1, t2), ms(t2, t3), ms(t3, t4), ms(t4, t5), ms(t0, t5));
  }
}