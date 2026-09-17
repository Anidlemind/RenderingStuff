#include "assets/mtl_loader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "render/color_space.h"

std::vector<MeshMaterial> LoadMtl(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Cannot open MTL: " + path.string());
  }
  std::vector<MeshMaterial> materials;
  std::string line;
  size_t line_number = 0;
  try {
    while (std::getline(input, line)) {
      ++line_number;
      line = line.substr(0, line.find('#'));
      std::istringstream fields(line);
      std::string tag;
      fields >> tag;
      if (tag.empty()) {
        continue;
      }
      if (tag == "newmtl") {
        std::string name;
        if (!(fields >> name)) {
          throw std::runtime_error("missing material name");
        }
        for (const auto& material : materials) {
          if (material.name == name) {
            throw std::runtime_error("duplicate material: " + name);
          }
        }
        materials.push_back({name, {{255, 255, 255}, {}, 1}, nullptr});
      } else if (tag == "Kd" || tag == "Ks" || tag == "Ns" || tag == "map_Kd") {
        if (materials.empty()) {
          throw std::runtime_error("property before newmtl");
        }
        auto& material = materials.back();
        if (tag == "Kd" || tag == "Ks") {
          Vec3 color;
          if (!(fields >> color.x >> color.y >> color.z) ||
              !std::isfinite(color.x) || !std::isfinite(color.y) ||
              !std::isfinite(color.z) || color.x < 0 || color.y < 0 ||
              color.z < 0 || color.x > 1 || color.y > 1 || color.z > 1) {
            throw std::runtime_error(
                "color must contain three values in [0,1]");
          }
          (tag == "Kd" ? material.surface.diffuse : material.surface.specular) =
              ToSrgb(color);
        } else if (tag == "Ns") {
          float shininess;
          if (!(fields >> shininess) || !std::isfinite(shininess) ||
              shininess < 0 || shininess > 1000) {
            throw std::runtime_error("Ns must be in [0,1000]");
          }
          material.surface.shininess = std::max(1.0f, shininess);
        } else {
          std::string filename;
          std::getline(fields >> std::ws, filename);
          const auto last = filename.find_last_not_of(" \t\r");
          if (last == std::string::npos) {
            throw std::runtime_error("missing map_Kd filename");
          }
          filename.resize(last + 1);
          if (filename.front() == '-') {
            throw std::runtime_error("map_Kd options are not supported");
          }
          const auto texture_path = path.parent_path() / filename;
          auto texture =
              std::make_shared<Texture>(LoadTexture(texture_path.string()));
          if (texture->pixels.empty()) {
            throw std::runtime_error("Cannot load map_Kd: " +
                                     texture_path.string());
          }
          material.texture = std::move(texture);
        }
      }
    }
    if (input.bad()) {
      throw std::runtime_error("failed to read MTL");
    }
  } catch (const std::runtime_error& error) {
    throw std::runtime_error(path.string() + ": line " +
                             std::to_string(line_number) + ": " + error.what());
  }
  return materials;
}
