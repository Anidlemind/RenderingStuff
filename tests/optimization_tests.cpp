#include <gtest/gtest.h>

#include <random>

#include "render/software_renderer.h"

TEST(OptimizedRasterization,
     MatchesDirectEdgeReferenceForRandomTrianglesAndRegions) {
  std::mt19937 random(173);
  std::uniform_int_distribution<int> coordinate(-1024, 6000);
  constexpr int kWidth = 17, kHeight = 13;
  for (int iteration = 0; iteration < 500; ++iteration) {
    const auto vertex = [&] {
      return Vertex2D{coordinate(random) / 256.0f,
                      coordinate(random) / 256.0f,
                      0,
                      {255, 255, 255}};
    };
    const auto prepared = PrepareTriangle(vertex(), vertex(), vertex());
    if (!prepared) {
      continue;
    }
    FrameBuffer frame(kWidth, kHeight);
    frame.Clear({});
    // Partial rectangles test both coarse rejection and full coverage paths.
    for (int y = 0; y < kHeight; y += 4) {
      for (int x = 0; x < kWidth; x += 4) {
        frame.DrawPreparedTriangle(
            *prepared,
            {x, std::min(x + 3, kWidth - 1), y, std::min(y + 3, kHeight - 1)});
      }
    }
    const auto edge = [](const Vertex2D& a, const Vertex2D& b, int x, int y) {
      const int64_t ax = std::llround(a.x * 256.0),
                    ay = std::llround(a.y * 256.0);
      const int64_t bx = std::llround(b.x * 256.0),
                    by = std::llround(b.y * 256.0);
      const auto value =
          (bx - ax) * (y * 256 + 128 - ay) - (by - ay) * (x * 256 + 128 - ax);
      const bool inclusive = by < ay || (by == ay && bx > ax);
      return value > 0 || (value == 0 && inclusive);
    };
    for (int y = 0; y < kHeight; ++y) {
      for (int x = 0; x < kWidth; ++x) {
        const bool inside = edge(prepared->a, prepared->b, x, y) &&
                            edge(prepared->b, prepared->c, x, y) &&
                            edge(prepared->c, prepared->a, x, y);
        ASSERT_EQ(frame.Pixels()[y * kWidth + x], inside ? 0xffffffu : 0u)
            << "triangle=" << iteration << " x=" << x << " y=" << y;
      }
    }
  }
}

TEST(OptimizedBinning, ReusesStorageAcrossResolutionsAndPreservesDepthTies) {
  Mesh scene;
  scene.vertices = {{-1, -1, 0, {255, 0, 0}}, {1, -1, 0, {255, 0, 0}},
                    {0, 1, 0, {255, 0, 0}},   {-1, -1, 0, {0, 255, 0}},
                    {1, -1, 0, {0, 255, 0}},  {0, 1, 0, {0, 255, 0}}};
  scene.triangles = {{0, 1, 2}, {3, 4, 5}};
  SoftwareRenderer serial(1), parallel(4);
  RenderSettings settings;
  settings.ambient = 1;
  for (int size : {1, 17, 65, 7, 32}) {
    FrameBuffer reference(size, size + 2), actual(size, size + 2);
    settings.tile_size = 0;
    serial.Render(scene, {}, settings, reference);
    for (int tile : {16, 32, 0}) {
      settings.tile_size = tile;
      parallel.Render(scene, {}, settings, actual);
      EXPECT_TRUE(std::equal(reference.Pixels(),
                             reference.Pixels() + size * (size + 2),
                             actual.Pixels()));
    }
    settings.tile_size = 8;
    EXPECT_THROW(parallel.Render(scene, {}, settings, actual),
                 std::invalid_argument);
  }
}
