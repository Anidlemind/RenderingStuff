#include "assets/obj_loader.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "assets/mtl_loader.h"
#include "math/math.h"

namespace {

struct VertexKey {
  int v = -1;
  int vt = -1;
  int vn = -1;
  int smoothing = 0;
  bool operator==(const VertexKey& o) const {
    return v == o.v && vt == o.vt && vn == o.vn && smoothing == o.smoothing;
  }
};

struct VertexKeyHash {
  size_t operator()(const VertexKey& k) const {
    return (static_cast<size_t>(k.v) * 73856093u) ^
           (static_cast<size_t>(k.vt) * 19349663u) ^
           (static_cast<size_t>(k.vn) * 83492791u) ^
           (static_cast<size_t>(k.smoothing) * 2654435761u);
  }
};

struct FaceIndex {
  int v = -1;
  int vt = -1;
  int vn = -1;
};

int ParseIndex(const std::string& s, int count) {
  if (s.empty()) {
    return -1;
  }
  int idx = 0;
  const auto [end, error] = std::from_chars(s.data(), s.data() + s.size(), idx);
  if (error != std::errc{} || end != s.data() + s.size() || idx == 0 ||
      idx > count || idx < -count) {
    throw std::runtime_error("invalid OBJ index: " + s);
  }
  return idx > 0 ? idx - 1 : count + idx;
}

FaceIndex ParseFaceToken(const std::string& tok, int v_count, int vt_count,
                         int vn_count) {
  FaceIndex fi;
  const size_t s1 = tok.find('/');
  if (s1 == std::string::npos) {
    fi.v = ParseIndex(tok, v_count);
    return fi;
  }
  fi.v = ParseIndex(tok.substr(0, s1), v_count);

  const size_t s2 = tok.find('/', s1 + 1);
  if (s2 == std::string::npos) {
    fi.vt = ParseIndex(tok.substr(s1 + 1), vt_count);
    return fi;
  }
  fi.vt = ParseIndex(tok.substr(s1 + 1, s2 - s1 - 1), vt_count);
  fi.vn = ParseIndex(tok.substr(s2 + 1), vn_count);
  return fi;
}

}  // namespace

namespace {
Mesh LoadObjInternal(std::istream& file,
                     const std::filesystem::path* directory);
}

Mesh LoadObj(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Cannot open OBJ: " + path);
  }
  try {
    const auto directory = std::filesystem::path(path).parent_path();
    return LoadObjInternal(file, &directory);
  } catch (const std::runtime_error& error) {
    throw std::runtime_error(path + ": " + error.what());
  }
}

Mesh LoadObj(std::istream& file) { return LoadObjInternal(file, nullptr); }

namespace {
Mesh LoadObjInternal(std::istream& file,
                     const std::filesystem::path* directory) {
  Mesh mesh;

  std::vector<Vec3> positions;
  std::vector<Vec3> normals;
  std::vector<Vec2> uvs;

  std::unordered_map<VertexKey, int, VertexKeyHash> unique_verts;
  std::vector<VertexKey> normal_keys;
  std::vector<bool> missing_normals;
  // Double area sums avoid overflow/underflow for valid extreme float
  // positions.
  std::unordered_map<VertexKey, std::array<double, 3>, VertexKeyHash>
      normal_sums;
  std::unordered_map<std::string, int> material_indices;
  std::unordered_map<std::string, int> smoothing_groups;
  int smoothing = 0;
  int face_number = 0;
  int current_material = -1;
  std::vector<std::filesystem::path> libraries;

  const auto material_index = [&](const std::string& name) {
    const auto found = material_indices.find(name);
    if (found != material_indices.end()) {
      return found->second;
    }
    const int index = static_cast<int>(mesh.materials.size());
    mesh.materials.push_back({name, {{255, 255, 255}, {}, 1}, nullptr});
    material_indices.emplace(name, index);
    return index;
  };

  auto get_or_create = [&](const FaceIndex& fi) -> int {
    const bool missing =
        fi.vn < 0 || (normals[fi.vn].x == 0 && normals[fi.vn].y == 0 &&
                      normals[fi.vn].z == 0);
    VertexKey key{fi.v, fi.vt, fi.vn,
                  missing ? (smoothing == 0 ? -face_number : smoothing) : 0};
    auto it = unique_verts.find(key);
    if (it != unique_verts.end()) {
      return it->second;
    }

    Vertex3D v;
    if (fi.v >= 0 && fi.v < static_cast<int>(positions.size())) {
      v.x = positions[fi.v].x;
      v.y = positions[fi.v].y;
      v.z = positions[fi.v].z;
    }
    if (fi.vn >= 0 && fi.vn < static_cast<int>(normals.size())) {
      v.normal = normals[fi.vn];
    }
    if (fi.vt >= 0 && fi.vt < static_cast<int>(uvs.size())) {
      v.uv = uvs[fi.vt];
    }
    v.color = {255, 255, 255};

    const int new_idx = static_cast<int>(mesh.vertices.size());
    mesh.vertices.push_back(v);
    normal_keys.push_back({fi.v, -1, -1, key.smoothing});
    missing_normals.push_back(missing);
    unique_verts.emplace(key, new_idx);
    return new_idx;
  };

  std::string line;
  size_t line_number = 0;
  try {
    while (std::getline(file, line)) {
      ++line_number;
      if (line.empty() || line[0] == '#') {
        continue;
      }

      std::istringstream iss(line);
      std::string tag;
      iss >> tag;

      if (tag == "v") {
        float x, y, z;
        if (!(iss >> x >> y >> z) || !std::isfinite(x) || !std::isfinite(y) ||
            !std::isfinite(z)) {
          throw std::runtime_error("invalid position");
        }
        positions.push_back({x, y, z});
      } else if (tag == "vn") {
        float x, y, z;
        if (!(iss >> x >> y >> z) || !std::isfinite(x) || !std::isfinite(y) ||
            !std::isfinite(z)) {
          throw std::runtime_error("invalid normal");
        }
        normals.push_back({x, y, z});
      } else if (tag == "vt") {
        float u, v;
        if (!(iss >> u >> v) || !std::isfinite(u) || !std::isfinite(v)) {
          throw std::runtime_error("invalid texture coordinate");
        }
        uvs.push_back({u, v});
      } else if (tag == "mtllib") {
        std::string name;
        bool any = false;
        while (iss >> name && name.front() != '#') {
          if (!directory) {
            throw std::runtime_error("mtllib requires loading OBJ by path");
          }
          libraries.push_back(*directory / name);
          any = true;
        }
        if (!any) {
          throw std::runtime_error("missing mtllib filename");
        }
      } else if (tag == "usemtl") {
        std::string name;
        if (!(iss >> name) || name.front() == '#') {
          throw std::runtime_error("missing material name");
        }
        current_material = material_index(name);
      } else if (tag == "s") {
        std::string name;
        if (!(iss >> name)) {
          throw std::runtime_error("missing smoothing group");
        }
        if (name == "off" || name == "0") {
          smoothing = 0;
        } else {
          const auto [it, inserted] = smoothing_groups.emplace(
              name, static_cast<int>(smoothing_groups.size()) + 1);
          smoothing = it->second;
        }
      } else if (tag == "f") {
        ++face_number;
        std::vector<int> face_idx;
        std::string tok;
        while (iss >> tok) {
          if (tok.front() == '#') {
            break;
          }
          const FaceIndex fi = ParseFaceToken(
              tok, static_cast<int>(positions.size()),
              static_cast<int>(uvs.size()), static_cast<int>(normals.size()));
          if (fi.v < 0) {
            throw std::runtime_error("missing position index");
          }
          face_idx.push_back(get_or_create(fi));
        }
        if (face_idx.size() < 3) {
          throw std::runtime_error("face requires at least three vertices");
        }
        for (size_t i = 2; i < face_idx.size(); ++i) {
          mesh.triangles.push_back({face_idx[0], face_idx[i - 1], face_idx[i]});
          mesh.triangle_materials.push_back(current_material);
          const auto edge = [&](int index) {
            const auto& v = mesh.vertices[index];
            const auto& origin = mesh.vertices[face_idx[0]];
            return std::array<double, 3>{static_cast<double>(v.x) - origin.x,
                                         static_cast<double>(v.y) - origin.y,
                                         static_cast<double>(v.z) - origin.z};
          };
          const auto a = edge(face_idx[i - 1]), b = edge(face_idx[i]);
          const std::array<double, 3> normal{a[1] * b[2] - a[2] * b[1],
                                             a[2] * b[0] - a[0] * b[2],
                                             a[0] * b[1] - a[1] * b[0]};
          for (int index : mesh.triangles.back()) {
            if (missing_normals[index]) {
              auto& sum = normal_sums[normal_keys[index]];
              for (int axis = 0; axis < 3; ++axis) {
                sum[axis] += normal[axis];
              }
            }
          }
        }
      }
    }
    if (file.bad()) {
      throw std::runtime_error("failed to read OBJ stream");
    }
  } catch (const std::runtime_error& error) {
    throw std::runtime_error("line " + std::to_string(line_number) + ": " +
                             error.what());
  }

  for (size_t i = 0; i < mesh.vertices.size(); ++i) {
    if (missing_normals[i]) {
      const auto& sum = normal_sums[normal_keys[i]];
      const double magnitude =
          std::max({std::abs(sum[0]), std::abs(sum[1]), std::abs(sum[2])});
      if (magnitude > 0) {
        mesh.vertices[i].normal =
            Normalize(Vec3{static_cast<float>(sum[0] / magnitude),
                           static_cast<float>(sum[1] / magnitude),
                           static_cast<float>(sum[2] / magnitude)});
      }
    }
  }
  std::unordered_map<std::string, bool> defined_materials;
  for (const auto& library : libraries) {
    for (auto& material : LoadMtl(library)) {
      if (!defined_materials.emplace(material.name, true).second) {
        throw std::runtime_error("Duplicate MTL material: " + material.name);
      }
      const int index = material_index(material.name);
      mesh.materials[index] = std::move(material);
    }
  }
  if (!libraries.empty()) {
    for (const auto& material : mesh.materials) {
      if (!defined_materials.contains(material.name)) {
        throw std::runtime_error("Undefined MTL material: " + material.name);
      }
    }
  }
  return mesh;
}
}  // namespace
