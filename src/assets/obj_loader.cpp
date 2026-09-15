#include "obj_loader.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "math/math.h"

namespace {

struct VertexKey {
  int v = -1;
  int vt = -1;
  int vn = -1;
  bool operator==(const VertexKey& o) const {
    return v == o.v && vt == o.vt && vn == o.vn;
  }
};

struct VertexKeyHash {
  size_t operator()(const VertexKey& k) const {
    return (static_cast<size_t>(k.v)  * 73856093u)
         ^ (static_cast<size_t>(k.vt) * 19349663u)
         ^ (static_cast<size_t>(k.vn) * 83492791u);
  }
};

struct FaceIndex {
  int v = -1;
  int vt = -1;
  int vn = -1;
};

int parseIndex(const std::string& s, int count) {
  if (s.empty()) return -1;
  int idx = 0;
  try { idx = std::stoi(s); } catch (...) { return -1; }
  if (idx > 0) return idx - 1;
  if (idx < 0) return count + idx;
  return -1;
}

FaceIndex parseFaceToken(const std::string& tok, int vCount, int vtCount, int vnCount) {
  FaceIndex fi;
  const size_t s1 = tok.find('/');
  if (s1 == std::string::npos) {
    fi.v = parseIndex(tok, vCount);
    return fi;
  }
  fi.v = parseIndex(tok.substr(0, s1), vCount);

  const size_t s2 = tok.find('/', s1 + 1);
  if (s2 == std::string::npos) {
    fi.vt = parseIndex(tok.substr(s1 + 1), vtCount);
    return fi;
  }
  fi.vt = parseIndex(tok.substr(s1 + 1, s2 - s1 - 1), vtCount);
  fi.vn = parseIndex(tok.substr(s2 + 1), vnCount);
  return fi;
}

}  // namespace

ObjMesh loadOBJ(const std::string& path) {
  ObjMesh mesh;

  std::ifstream file(path);
  if (!file) {
    std::fprintf(stderr, "loadOBJ: cannot open '%s'\n", path.c_str());
    return mesh;
  }

  std::vector<Vec3> positions;
  std::vector<Vec3> normals;
  std::vector<Vec2> uvs;

  std::unordered_map<VertexKey, int, VertexKeyHash> uniqueVerts;
  uniqueVerts.reserve(1 << 20);

  auto getOrCreate = [&](const FaceIndex& fi) -> int {
    VertexKey key{fi.v, fi.vt, fi.vn};
    auto it = uniqueVerts.find(key);
    if (it != uniqueVerts.end()) return it->second;

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

    const int newIdx = static_cast<int>(mesh.verts.size());
    mesh.verts.push_back(v);
    uniqueVerts.emplace(key, newIdx);
    return newIdx;
  };

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') continue;

    std::istringstream iss(line);
    std::string tag;
    iss >> tag;

    if (tag == "v") {
      float x, y, z;
      iss >> x >> y >> z;
      positions.push_back({x, y, z});
    } else if (tag == "vn") {
      float x, y, z;
      iss >> x >> y >> z;
      normals.push_back({x, y, z});
    } else if (tag == "vt") {
      float u, v;
      iss >> u >> v;
      uvs.push_back({u, v});
    } else if (tag == "f") {
      std::vector<int> faceIdx;
      std::string tok;
      while (iss >> tok) {
        const FaceIndex fi = parseFaceToken(
          tok,
          static_cast<int>(positions.size()),
          static_cast<int>(uvs.size()),
          static_cast<int>(normals.size()));
        if (fi.v >= 0) {
          faceIdx.push_back(getOrCreate(fi));
        }
      }
      for (size_t i = 2; i < faceIdx.size(); ++i) {
        mesh.tris.push_back({faceIdx[0], faceIdx[i - 1], faceIdx[i]});
      }
    }
  }

  std::fprintf(stderr, "loadOBJ: '%s' — %zu verts, %zu tris, uvs=%zu, normals=%zu\n",
               path.c_str(), mesh.verts.size(), mesh.tris.size(),
               uvs.size(), normals.size());

  return mesh;
}