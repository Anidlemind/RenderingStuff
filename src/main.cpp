#include "app/options.h"
#ifdef RENDERER_WITH_SDL
#include "app/app.h"
#endif
#include <cstdio>
#include <exception>

int main(int argc, char** argv) {
  try {
#ifdef RENDERER_WITH_SDL
    constexpr bool kHeadlessOnly = false;
#else
    constexpr bool kHeadlessOnly = true;
#endif
    const auto options = ParseOptions(argc, argv, kHeadlessOnly);
    if (options.help) {
      std::fputs(Usage(), stdout);
      return 0;
    }
    if (options.headless) {
      return RunHeadless(options);
    }
#ifdef RENDERER_WITH_SDL
    App app(options);
    return app.Run();
#endif
  } catch (const std::exception& error) {
    std::fprintf(stderr, "renderer: %s\n", error.what());
    return 1;
  }
}
