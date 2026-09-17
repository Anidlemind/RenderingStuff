#ifndef RENDERER_SRC_SCENE_BENCHMARK_SCENES_H_
#define RENDERER_SRC_SCENE_BENCHMARK_SCENES_H_

#include <string_view>

#include "render/software_renderer.h"

struct BenchmarkScene {
  Mesh mesh;
  Mat4 model;
  Camera camera;
  Texture texture;
  RenderSettings settings;
  bool primitives = false;
  Scene Instantiate() const;
};

// Versioned, deterministic fixtures. Camera and geometry must change together
// with the benchmark suite version when intentionally updating the baseline.
inline constexpr int kBenchmarkSuiteVersion = 2;
BenchmarkScene MakeBenchmarkScene(std::string_view name);

#endif  // RENDERER_SRC_SCENE_BENCHMARK_SCENES_H_
