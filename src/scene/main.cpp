#include "render/frame_buffer.h"
#include "render/projection.h"
#include "math/math.h"

#include <array>
#include <iostream>
#include <string>

int main() {
  constexpr uint32_t width = 800;
  constexpr uint32_t height = 600;

  FrameBuffer fb(width, height);
  
  // // Grad
  // for (uint32_t y = 0; y < height; ++y) {
  //   for (uint32_t x = 0; x < width; ++x) {
  //     uint8_t r = static_cast<uint8_t>(255 * x / (width - 1));
  //     uint8_t g = static_cast<uint8_t>(255 * y / (height - 1));
  //     uint8_t b = 128;

  //     fb.setPixel(x, y, {r, g, b});
  //   }
  // }

  // // Lines
  // fb.drawLine(0, height / 2, width - 1, height / 2, {0, 0, 0});
  // fb.drawLine(width / 2, 0, width / 2, height - 1, {0, 0, 0});
  // fb.drawLine(0, 0, width - 1, height - 1, {0, 0, 0});
  // fb.drawLine(width - 1, 0, 0, height - 1, {0, 0, 0});

  // // Triangle
  // fb.drawTriangle(
  //   {400.0f, 100.0f, 0.0f, {255, 0, 0}},   // top, red
  //   {100.0f, 500.0f, 0.0f, {0, 255, 0}},   // bottom-left, green
  //   {700.0f, 500.0f, 0.0f, {0, 0, 255}}    // bottom-right, blue
  // );
  auto phi = 1.618f;
  
  std::vector<Vertex3D> verts = {
    // (±1, ±φ, 0)
    {-1,  phi,  0, {255,   0,   0}},
    { 1,  phi,  0, {255,  64,   0}},
    {-1, -phi,  0, {255, 128,   0}},
    { 1, -phi,  0, {255, 192,   0}},

    // (0, ±1, ±φ)
    { 0, -1,  phi, {128, 255,   0}},
    { 0,  1,  phi, {  0, 255,   0}},
    { 0, -1, -phi, {  0, 255, 128}},
    { 0,  1, -phi, {  0, 255, 255}},

    // (±φ, 0, ±1)
    { phi,  0, -1, {  0, 128, 255}},
    { phi,  0,  1, {  0,   0, 255}},
    {-phi,  0, -1, {128,   0, 255}},
    {-phi,  0,  1, {255,   0, 255}},
  };

  std::vector<std::array<int, 3>> tris = {
    // вокруг вершины 0 (-1, φ, 0)
    { 0, 11,  5},
    { 0,  5,  1},
    { 0,  1,  7},
    { 0,  7, 10},
    { 0, 10, 11},

    // вокруг вершины 3 (1, -φ, 0)
    { 3,  9,  4},
    { 3,  4,  2},
    { 3,  2,  6},
    { 3,  6,  8},
    { 3,  8,  9},

    // вокруг вершины 5 (0, 1, φ)
    { 5, 11,  4},   // исправлено с {11, 5, 4} — порядок не важен, winding нормализуется
    { 5,  4,  9},   // исправлено с {9, 4, 5}

    // вокруг вершины 7 (0, 1, -φ)
    { 7,  8, 10},   // исправлено с {8, 7, 10}
    { 7, 10,  6},   // исправлено с {6, 7, 10}

    // вокруг вершины 2 (-1, -φ, 0)
    { 2,  4, 11},   // исправлено с {4, 11, 2}
    { 2, 11, 10},   // исправлено с {10, 11, 2}

    // вокруг вершины 8 (φ, 0, -1)
    { 8,  6, 10},   // исправлено с {6, 8, 10}

    // вокруг вершины 9 (φ, 0, 1)
    { 9,  1,  5},   // исправлено с {1, 9, 5}

    // вокруг вершины 6 (0, -1, -φ)
    { 6,  7,  8},   // исправлено с {7, 8, 6}

    // вокруг вершины 4 (0, -1, φ)
    { 4,  3,  9},   // исправлено с {3, 4, 9} — дубликат?
  };

  fb.clear({20, 20, 30});   // тёмно-синий фон

  float angle = radians(30.0f);

  Mat4 model = rotationY(angle) * rotationX(radians(20.0f));
  Mat4 view  = lookAt({0, 0, 5}, {0, 0, 0}, {0, 1, 0});
  Mat4 proj  = perspective(radians(60.0f),
                          static_cast<float>(width) / height,
                          0.1f, 100.0f);
  Mat4 mvp = proj * view * model;

  for (const auto& tri : tris) {
    Vertex2D a, b, c;
    bool ok_a = project(verts[tri[0]], mvp, width, height, a);
    bool ok_b = project(verts[tri[1]], mvp, width, height, b);
    bool ok_c = project(verts[tri[2]], mvp, width, height, c);

    if (!ok_a || !ok_b || !ok_c) {
      continue;   // грубое отсечение: если хоть одна вершина за камерой — пропускаем
    }

    fb.drawTriangle(a, b, c);
  }

  if (!fb.savePPM("./out.ppm")) {
    std::cerr << "Failed to save out.ppm\n";
  }

  std::cerr << "Saved out.ppm\n";
}