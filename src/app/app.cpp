#include "app/app.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "math/math.h"
#include "render/projection.h"
#include "render/text.h"

namespace {
  constexpr int kWidth  = 800;
  constexpr int kHeight = 600;

  constexpr Vec3 kLightDir{0.5f, 1.0f, 0.5f};

  constexpr float kAmbient = 0.15f;
}

App::App()
  : window_("renderer", kWidth, kHeight),
    framebuffer_(static_cast<uint32_t>(kWidth), static_cast<uint32_t>(kHeight)),
    camera_(),
    cameraController_(),
    scene_(Scene::makeIcosahedron()) {
  camera_.position = {0.0f, 0.0f, 5.0f};
  camera_.yaw      = 0.0f;
  camera_.pitch    = 0.0f;
  camera_.fovY     = radians(60.0f);
  camera_.near     = 0.1f;
  camera_.far      = 100.0f;

  pixelBuffer_.resize(static_cast<size_t>(kWidth) * kHeight);
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

    const float clampedDt = std::min(dt, 0.1f);

    if (clampedDt > 1e-6f) {
      const float instant = 1.0f / clampedDt;
      if (smoothedFps_ <= 0.0f) {
        smoothedFps_ = instant;
      } else {
        smoothedFps_ = smoothedFps_ * 0.95f + instant * 0.05f;
      }
    }

    update(clampedDt);
    render();
  }

  return 0;
}

void App::update(float dt) {
  cameraController_.update(camera_, window_.input(), dt);
}

void App::render() {
  framebuffer_.clear({20, 20, 30});

  const float aspect = static_cast<float>(kWidth) / static_cast<float>(kHeight);

  const Mat4 mvp = camera_.viewProjection(aspect) * scene_.model;

  const Vec3 lightDir = normalize(kLightDir);

  for (const auto& tri : scene_.tris) {
    const Vertex3D& v0 = scene_.verts[tri[0]];
    const Vertex3D& v1 = scene_.verts[tri[1]];
    const Vertex3D& v2 = scene_.verts[tri[2]];

    const Vec4 w0 = scene_.model * Vec4{v0.x, v0.y, v0.z, 1.0f};
    const Vec4 w1 = scene_.model * Vec4{v1.x, v1.y, v1.z, 1.0f};
    const Vec4 w2 = scene_.model * Vec4{v2.x, v2.y, v2.z, 1.0f};

    const Vec3 p0{w0.x, w0.y, w0.z};
    const Vec3 p1{w1.x, w1.y, w1.z};
    const Vec3 p2{w2.x, w2.y, w2.z};

    Vec3 N = normalize(cross(p1 - p0, p2 - p0));

    const Vec3 viewDir = normalize(camera_.position - p0);
    if (dot(N, viewDir) < 0.0f) {
      N = -N;
    }

    const float diff = std::max(0.0f, dot(N, lightDir));
    const float intensity = kAmbient + (1.0f - kAmbient) * diff;

    const Color& base = v0.color;
    const Color lit{
      static_cast<uint8_t>(std::min(255.0f, static_cast<float>(base.r) * intensity)),
      static_cast<uint8_t>(std::min(255.0f, static_cast<float>(base.g) * intensity)),
      static_cast<uint8_t>(std::min(255.0f, static_cast<float>(base.b) * intensity))
    };

    Vertex2D a, b, c;
    const bool ok_a = project(v0, mvp, kWidth, kHeight, a);
    const bool ok_b = project(v1, mvp, kWidth, kHeight, b);
    const bool ok_c = project(v2, mvp, kWidth, kHeight, c);

    if (!ok_a || !ok_b || !ok_c) {
      continue;
    }

    a.color = lit;
    b.color = lit;
    c.color = lit;

    framebuffer_.drawTriangle(a, b, c);
  }

  {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "FPS: %d",
                  static_cast<int>(smoothedFps_ + 0.5f));

    drawText(framebuffer_, 11, 11, buf, {0, 0, 0});
    drawText(framebuffer_, 10, 10, buf, {255, 255, 255});
  }

  window_.present(framebuffer_.pixels(), kWidth, kHeight);
}