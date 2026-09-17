#ifndef RENDERER_SRC_ASSETS_MTL_LOADER_H_
#define RENDERER_SRC_ASSETS_MTL_LOADER_H_

#include <filesystem>
#include <vector>

#include "scene/mesh.h"

// Supported opaque subset: newmtl, Kd, Ks, Ns and map_Kd (without options).
// Unknown directives are ignored. Malformed supported values throw with
// location.
std::vector<MeshMaterial> LoadMtl(const std::filesystem::path& path);

#endif  // RENDERER_SRC_ASSETS_MTL_LOADER_H_
