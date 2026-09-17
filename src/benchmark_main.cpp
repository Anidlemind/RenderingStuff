#include <array>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "core/sample_statistics.h"
#include "scene/benchmark_scenes.h"

int main(int argc, char** argv) {
  try {
    int width = 640, height = 480, workers = 1, warmup = 20, frames = 100,
        tile_size = 0;
    std::string scene_name = "all", report, shading_name = "legacy",
                filter_name = "trilinear", aa_name = "none";
    bool shadows = false;
    int shadow_size = 1024;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--shadows") {
        shadows = true;
        continue;
      }
      if (arg == "--help") {
        std::cout << "renderer_benchmark [--scene "
                     "all|quad|overlap|clipped|micro|shrimp|primitives]\n"
                     "  [--width N] [--height N] [--workers N] [--warmup N] "
                     "[--frames N] [--report file.csv] [--tile-size 0|16|32]\n"
                     "  [--shading legacy|phong|unlit] [--filter "
                     "nearest|bilinear|trilinear] [--aa none|ssaa4] "
                     "[--shadows] [--shadow-size N]\n"
                     "Defaults: 640x480, 1 worker, 20 warm-up frames, 100 "
                     "measured frames.\n";
        return 0;
      }
      if (++i >= argc) {
        throw std::invalid_argument("Missing value for " + arg);
      }
      const std::string value = argv[i];
      const auto number = [&](int low, int high) {
        int result = 0;
        const auto [end, error] =
            std::from_chars(value.data(), value.data() + value.size(), result);
        if (error != std::errc{} || end != value.data() + value.size() ||
            result < low || result > high) {
          throw std::invalid_argument("Invalid value for " + arg);
        }
        return result;
      };
      if (arg == "--width") {
        width = number(1, 16384);
      } else if (arg == "--height") {
        height = number(1, 16384);
      } else if (arg == "--workers") {
        workers = number(0, 256);
      } else if (arg == "--warmup") {
        warmup = number(0, 1'000'000);
      } else if (arg == "--shadow-size") {
        shadow_size = number(16, 4096);
      } else if (arg == "--frames") {
        frames = number(1, 1'000'000);
      } else if (arg == "--tile-size") {
        tile_size = number(0, 32);
        if (tile_size != 0 && tile_size != 16 && tile_size != 32) {
          throw std::invalid_argument("Tile size must be 0, 16 or 32");
        }
      } else if (arg == "--scene") {
        scene_name = value;
      } else if (arg == "--shading") {
        ParseShadingMode(value);
        shading_name = value;
      } else if (arg == "--filter") {
        ParseTextureFilter(value);
        filter_name = value;
      } else if (arg == "--report") {
        report = value;
      } else if (arg == "--aa") {
        ParseAntialiasing(value);
        aa_name = value;
      } else {
        throw std::invalid_argument("Unknown option: " + arg);
      }
    }
    if (aa_name == "ssaa4" && (width > 8192 || height > 8192)) {
      throw std::invalid_argument("SSAA output dimensions exceed 8192");
    }
    const std::array<std::string, 6> names{"quad",  "overlap", "clipped",
                                           "micro", "shrimp",  "primitives"};
    if (scene_name != "all" &&
        std::find(names.begin(), names.end(), scene_name) == names.end()) {
      throw std::invalid_argument("Unknown benchmark scene: " + scene_name);
    }
    std::ofstream file;
    if (!report.empty()) {
      file.open(report);
      if (!file) {
        throw std::runtime_error("Cannot open report: " + report);
      }
    }
    std::ostream& output = report.empty() ? std::cout : file;
    output << "suite,build,scene,width,height,workers,warmup,frames,input_"
              "triangles,raster_triangles,total_median_ms,total_p95_ms,clear_"
              "median_ms,clear_p95_ms,transform_median_ms,transform_p95_ms,"
              "setup_median_ms,setup_p95_ms,raster_median_ms,raster_p95_ms,"
              "tile_size,shading,filter,antialiasing,resolve_median_ms,resolve_"
              "p95_ms,shadows,shadow_size,shadow_median_ms,shadow_p95_ms,input_"
              "objects,culled_objects\n";
    output << std::fixed << std::setprecision(6);
    for (const auto& name : names) {
      if (scene_name != "all" && name != scene_name) {
        continue;
      }
      auto fixture = MakeBenchmarkScene(name);
      fixture.settings.tile_size = tile_size;
      fixture.settings.shading = ParseShadingMode(shading_name);
      fixture.settings.filter = ParseTextureFilter(filter_name);
      fixture.settings.antialiasing = ParseAntialiasing(aa_name);
      fixture.settings.shadows = shadows;
      fixture.settings.shadow_size = shadow_size;
      const auto scene = fixture.Instantiate();
      FrameBuffer target(width, height);
      SoftwareRenderer renderer(workers);
      const auto render = [&] {
        return renderer.Render(scene, fixture.camera, fixture.settings, target);
      };
      for (int i = 0; i < warmup; ++i) {
        render();
      }
      std::array<std::vector<double>, 7> samples;
      for (auto& sample : samples) {
        sample.reserve(frames);
      }
      RenderStats stats;
      for (int i = 0; i < frames; ++i) {
        const auto start = std::chrono::steady_clock::now();
        stats = render();
        const auto stop = std::chrono::steady_clock::now();
        samples[0].push_back(
            std::chrono::duration<double, std::milli>(stop - start).count());
        samples[1].push_back(stats.clear_ms);
        samples[2].push_back(stats.transform_ms);
        samples[3].push_back(stats.setup_ms);
        samples[4].push_back(stats.raster_ms);
        samples[5].push_back(stats.resolve_ms);
        samples[6].push_back(stats.shadow_ms);
      }
      output << kBenchmarkSuiteVersion << ',' << RENDERER_BUILD_CONFIG << ','
             << name << ',' << width << ',' << height << ','
             << renderer.WorkerCount() << ',' << warmup << ',' << frames << ','
             << stats.input_triangles << ',' << stats.raster_triangles;
      for (size_t i = 0; i < 5; ++i) {
        const auto summary = SummarizeSamples(samples[i]);
        output << ',' << summary.median << ',' << summary.p95;
      }
      output << ',' << tile_size << ',' << shading_name << ','
             << (shading_name == "legacy" ? "nearest" : filter_name) << ','
             << aa_name;
      const auto resolve = SummarizeSamples(samples[5]);
      const auto shadow = SummarizeSamples(samples[6]);
      output << ',' << resolve.median << ',' << resolve.p95 << ','
             << (shadows && shading_name == "phong") << ',' << shadow_size
             << ',' << shadow.median << ',' << shadow.p95 << ','
             << stats.input_objects << ',' << stats.culled_objects << '\n';
    }
    output.flush();
    if (!output) {
      throw std::runtime_error("Could not write benchmark report");
    }
    if (file.is_open()) {
      file.close();
      if (!file) {
        throw std::runtime_error("Could not close benchmark report");
      }
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "benchmark: " << error.what() << '\n';
    return 1;
  }
}
