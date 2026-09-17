#ifndef RENDERER_SRC_ASSETS_OBJ_LOADER_H_
#define RENDERER_SRC_ASSETS_OBJ_LOADER_H_

#include <iosfwd>
#include <string>

#include "scene/mesh.h"

Mesh LoadObj(const std::string& path);
// Malformed geometry throws std::runtime_error with a line number.
Mesh LoadObj(std::istream& input);

#endif  // RENDERER_SRC_ASSETS_OBJ_LOADER_H_
