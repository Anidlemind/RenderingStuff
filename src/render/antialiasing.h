#ifndef RENDERER_SRC_RENDER_ANTIALIASING_H_
#define RENDERER_SRC_RENDER_ANTIALIASING_H_

#include <string_view>

enum class Antialiasing { kNone, kSsaa4 };
Antialiasing ParseAntialiasing(std::string_view name);

#endif  // RENDERER_SRC_RENDER_ANTIALIASING_H_
