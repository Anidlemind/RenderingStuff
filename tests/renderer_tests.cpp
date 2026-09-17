#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "assets/obj_loader.h"
#include "assets/texture.h"
#include "core/thread_pool.h"
#include "render/frame_buffer.h"
#include "render/projection.h"
#include "scene/scene.h"

TEST(ObjLoader, TriangulatesNegativeIndicesAndPreservesSeams) {
  std::istringstream input(
      "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
      "vt 0 0\nvn 0 0 1\nf -4/1/1 -3/1/1 -2/1/1 -1/1/1 # quad\n"
      "f 1//1 2//1 3//1\n");
  const auto mesh = LoadObj(input);
  EXPECT_EQ(mesh.triangles.size(), 3u);
  ASSERT_EQ(mesh.vertices.size(), 7u);
  EXPECT_FLOAT_EQ(mesh.vertices[2].x, 1);
  EXPECT_FLOAT_EQ(mesh.vertices[2].y, 1);
  EXPECT_EQ(mesh.triangles[1], (std::array<int, 3>{0, 2, 3}));
}

class InvalidObjFace : public testing::TestWithParam<const char*> {};
TEST_P(InvalidObjFace, RejectsMalformedOrOutOfRangeIndices) {
  std::istringstream input(std::string("v 0 0 0\nv 1 0 0\nv 0 1 0\n") +
                           GetParam());
  EXPECT_THROW(LoadObj(input), std::runtime_error);
}
INSTANTIATE_TEST_SUITE_P(MalformedInput, InvalidObjFace,
                         testing::Values("f 0 2 3", "f 1 2 4", "f -4 2 3",
                                         "f 1junk 2 3", "f 1/1 2 3",
                                         "f 1//1 2 3", "f / 2 3", "f 1 2",
                                         "f 1//1/2 2 3"));

TEST(ObjLoader, ReportsLineNumberForMalformedPosition) {
  std::istringstream input("v 1 nope\n");
  try {
    LoadObj(input);
    FAIL() << "Malformed position accepted";
  } catch (const std::runtime_error& error) {
    EXPECT_NE(std::string(error.what()).find("line 1"), std::string::npos);
  }
}

TEST(Assets, LoadsBundledSceneAndTexture) {
  const auto scene = Mesh::LoadFromObj(RENDERER_TEST_ASSET_DIR "/Shrimp.obj");
  EXPECT_FALSE(scene.vertices.empty());
  EXPECT_FALSE(scene.triangles.empty());
  const auto texture =
      LoadTexture(RENDERER_TEST_ASSET_DIR "/Shrimp_texture.png");
  EXPECT_GT(texture.width, 0);
  EXPECT_GT(texture.height, 0);
  EXPECT_EQ(texture.pixels.size(),
            static_cast<size_t>(texture.width) * texture.height);
}

TEST(ThreadPool, RejectsInvalidWorkerCountAndEmptyTasks) {
  EXPECT_THROW(ThreadPool(0), std::invalid_argument);
  EXPECT_THROW(ThreadPool(-1), std::invalid_argument);
  ThreadPool pool(1);
  EXPECT_THROW(pool.Submit({}), std::invalid_argument);
}

TEST(ThreadPool, DrainsWorkPropagatesFailuresAndCanBeReused) {
  std::atomic<int> completed{0};
  ThreadPool pool(3);
  pool.Submit([] { throw std::runtime_error("task failed"); });
  for (int i = 0; i < 100; ++i) {
    pool.Submit([&] { ++completed; });
  }
  EXPECT_THROW(pool.Wait(), std::runtime_error);
  EXPECT_EQ(completed.load(), 100);
  pool.Submit([&] { ++completed; });
  EXPECT_NO_THROW(pool.Wait());
  EXPECT_EQ(completed.load(), 101);
}

TEST(ThreadPool, DestructorDrainsQueuedTasks) {
  std::atomic<int> completed{0};
  {
    ThreadPool pool(3);
    for (int i = 0; i < 100; ++i) {
      pool.Submit([&] { ++completed; });
    }
  }
  EXPECT_EQ(completed.load(), 100);
}

TEST(FrameBuffer, RejectsInvalidDimensions) {
  EXPECT_THROW(FrameBuffer(0, 4), std::invalid_argument);
  EXPECT_THROW(FrameBuffer(4, 0), std::invalid_argument);
  EXPECT_THROW(FrameBuffer(std::numeric_limits<uint32_t>::max(), 4),
               std::invalid_argument);
}

class Rasterization : public testing::Test {
 protected:
  FrameBuffer frame_{4, 4};
  Vertex2D a_{0, 0, 0.5f, {255, 0, 0}};
  Vertex2D b_{4, 0, 0.5f, {255, 0, 0}};
  Vertex2D c_{0, 4, 0.5f, {255, 0, 0}};
  void SetUp() override { frame_.Clear({0, 0, 0}); }
};

TEST_F(Rasterization, KeepsNearestVisibleFragment) {
  frame_.DrawTriangle(a_, b_, c_);
  ASSERT_EQ(frame_.Pixels()[0], 0xff0000u);
  frame_.BlendPixel(0, 0, 0.75f, {0, 255, 0});
  EXPECT_EQ(frame_.Pixels()[0], 0xff0000u);
  frame_.BlendPixel(0, 0, -2, {0, 255, 0});
  EXPECT_EQ(frame_.Pixels()[0], 0xff0000u);
  frame_.BlendPixel(0, 0, 0.25f, {0, 0, 255});
  EXPECT_EQ(frame_.Pixels()[0], 0x0000ffu);
}

TEST_F(Rasterization, SupportsEitherWindingAndIgnoresDegenerateTriangles) {
  frame_.DrawTriangle(c_, b_, a_);
  EXPECT_EQ(frame_.Pixels()[0], 0xff0000u);
  frame_.Clear({0, 0, 0});
  frame_.DrawTriangle(a_, a_, a_);
  frame_.SetPixel(-1, -1, {255, 255, 255});
  EXPECT_EQ(frame_.Pixels()[0], 0u);
}

TEST_F(Rasterization, RestrictsDrawingAndClearingToRequestedRows) {
  frame_.DrawTriangleRange(a_, b_, c_, 1, 1);
  EXPECT_EQ(frame_.Pixels()[0], 0u);
  EXPECT_EQ(frame_.Pixels()[4], 0xff0000u);
  frame_.ClearRange({1, 2, 3}, 1, 1);
  EXPECT_EQ(frame_.Pixels()[4], 0x010203u);
  EXPECT_EQ(frame_.Pixels()[0], 0u);
  EXPECT_EQ(frame_.Pixels()[8], 0u);
}

TEST(FrameBuffer, ExportsBinaryPpmWithRgbChannels) {
  FrameBuffer frame(1, 1);
  frame.Clear({1, 2, 255});
  const auto path = std::filesystem::current_path() / "renderer-test.ppm";
  ASSERT_TRUE(frame.SavePpm(path.string()));
  std::ifstream file(path, std::ios::binary);
  const std::string bytes((std::istreambuf_iterator<char>(file)), {});
  file.close();
  std::filesystem::remove(path);
  const std::string header = "P6\n1 1\n255\n";
  ASSERT_EQ(bytes.size(), header.size() + 3);
  EXPECT_TRUE(bytes.starts_with(header));
  EXPECT_EQ(static_cast<unsigned char>(bytes[header.size()]), 1);
  EXPECT_EQ(static_cast<unsigned char>(bytes[header.size() + 1]), 2);
  EXPECT_EQ(static_cast<unsigned char>(bytes[header.size() + 2]), 255);
  EXPECT_FALSE(frame.SavePpm("nonexistent-directory/image.ppm"));
}

TEST(Projection, MapsClipSpaceToViewport) {
  Vertex2D out;
  ASSERT_TRUE(ProjectFromClip({0, 0, 0, 1}, {}, {}, 100, 80, out));
  EXPECT_FLOAT_EQ(out.x, 50);
  EXPECT_FLOAT_EQ(out.y, 40);
  EXPECT_FLOAT_EQ(out.inv_w, 1);
}

TEST(Projection, RejectsInvalidInputs) {
  Vertex2D out;
  EXPECT_FALSE(ProjectFromClip({0, 0, 0, 0}, {}, {}, 100, 80, out));
  EXPECT_FALSE(ProjectFromClip({0, 0, 0, -1}, {}, {}, 100, 80, out));
  EXPECT_FALSE(ProjectFromClip({0, 0, 0, 1}, {}, {}, 0, 80, out));
  EXPECT_FALSE(
      ProjectFromClip({std::numeric_limits<float>::quiet_NaN(), 0, 0, 1}, {},
                      {}, 100, 80, out));
}

TEST(Texture, WrapsUvsAndHandlesInvalidData) {
  Texture texture{2, 1, {{255, 0, 0}, {0, 255, 0}}};
  EXPECT_EQ(texture.Sample(0.1f, 0).r, 255);
  EXPECT_EQ(texture.Sample(-0.25f, 0).g, 255);
  EXPECT_EQ(texture.Sample(1.1f, 0).r, 255);
  EXPECT_EQ(texture.Sample(std::numeric_limits<float>::quiet_NaN(), 0).b, 255);
  texture.pixels.clear();
  EXPECT_EQ(texture.Sample(0, 0).b, 255);
}
