#include <cstdio>
#include <stdexcept>

#include "app/options.h"
#include "render/software_renderer.h"

int RunHeadless(const AppOptions& options) {
  const auto scene = LoadScene(options);
  const auto camera = MakeCamera(options);
  FrameBuffer target(options.width, options.height);
  SoftwareRenderer renderer(options.workers);
  RenderStats stats;
  const auto settings = MakeRenderSettings(options);
  double elapsed = 0;
  for (int frame = 0; frame < options.frames; ++frame) {
    stats = renderer.Render(scene, camera, settings, target);
    elapsed += stats.clear_ms + stats.transform_ms + stats.setup_ms +
               stats.raster_ms + stats.resolve_ms + stats.shadow_ms;
  }
  if (!options.output.empty() && !target.SavePpm(options.output)) {
    throw std::runtime_error("Could not write image: " + options.output);
  }
  std::printf(
      "Rendered %d frame(s), %dx%d, %d workers, %zu input / %zu raster "
      "triangles, %.3f ms/frame; %zu/%zu objects culled; shadow %.3f / resolve "
      "%.3f ms\n",
      options.frames, options.width, options.height, renderer.WorkerCount(),
      stats.input_triangles, stats.raster_triangles, elapsed / options.frames,
      stats.culled_objects, stats.input_objects, stats.shadow_ms,
      stats.resolve_ms);
  return 0;
}
