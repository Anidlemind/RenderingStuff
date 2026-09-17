#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <random>

#include "app/options.h"
#include "render/clipping.h"
#include "render/software_renderer.h"

namespace {
constexpr Vec3 kWhite{255, 255, 255};
ClipVertex Vertex(Vec4 position) { return {position, {}, kWhite}; }

void DrawClipped(FrameBuffer& frame, ClipVertex a, ClipVertex b, ClipVertex c) {
  const auto polygon = ClipTriangle(a, b, c);
  for (size_t i = 2; i < polygon.size; ++i) {
    Vertex2D pa, pb, pc;
    ASSERT_TRUE(ProjectClippedVertex(polygon.vertices[0], frame.Width(),
                                     frame.Height(), pa));
    ASSERT_TRUE(ProjectClippedVertex(polygon.vertices[i - 1], frame.Width(),
                                     frame.Height(), pb));
    ASSERT_TRUE(ProjectClippedVertex(polygon.vertices[i], frame.Width(),
                                     frame.Height(), pc));
    frame.DrawTriangle(pa, pb, pc);
  }
}

Mesh TriangleScene() {
  Mesh scene;
  scene.vertices = {{-1, -1, 0, {255, 0, 0}},
                    {1, -1, 0, {0, 255, 0}},
                    {0, 1, 0, {0, 0, 255}}};
  scene.triangles = {{0, 1, 2}};
  return scene;
}

RenderSettings Unlit() {
  RenderSettings settings;
  settings.ambient = 1;
  settings.clear_color = {};
  return settings;
}

size_t ColoredPixels(const FrameBuffer& frame) {
  return std::count_if(frame.Pixels(),
                       frame.Pixels() + frame.Width() * frame.Height(),
                       [](uint32_t pixel) { return pixel != 0; });
}

AppOptions Options(std::initializer_list<const char*> args) {
  std::vector<std::string> storage{"renderer"};
  for (const auto* arg : args) {
    storage.emplace_back(arg);
  }
  std::vector<char*> argv;
  for (auto& arg : storage) {
    argv.push_back(arg.data());
  }
  return ParseOptions(static_cast<int>(argv.size()), argv.data(), false);
}
}  // namespace

class FrustumPlane : public testing::TestWithParam<int> {};
TEST_P(FrustumPlane, ClipsCrossingTrianglesAndRejectsOutsideTriangles) {
  auto a = Vertex({0, 0.5f, 0, 1});
  auto b = Vertex({-0.5f, -0.5f, 0, 1});
  auto c = Vertex({0.5f, -0.5f, 0, 1});
  const int axis = GetParam() / 2;
  const float outside = GetParam() % 2 == 0 ? -2.0f : 2.0f;
  const auto set = [axis, outside](ClipVertex& v) {
    if (axis == 0) {
      v.position.x = outside;
    } else if (axis == 1) {
      v.position.y = outside;
    } else {
      v.position.z = outside;
    }
  };
  set(a);
  const auto polygon = ClipTriangle(a, b, c);
  ASSERT_GE(polygon.size, 3u);
  for (size_t i = 0; i < polygon.size; ++i) {
    const auto p = polygon.vertices[i].position;
    EXPECT_GE(p.x, -p.w);
    EXPECT_LE(p.x, p.w);
    EXPECT_GE(p.y, -p.w);
    EXPECT_LE(p.y, p.w);
    EXPECT_GE(p.z, -p.w);
    EXPECT_LE(p.z, p.w);
  }
  set(b);
  set(c);
  EXPECT_EQ(ClipTriangle(a, b, c).size, 0u);
}
INSTANTIATE_TEST_SUITE_P(AllSixPlanes, FrustumPlane, testing::Range(0, 6));

TEST(Clipping, InterpolatesAttributesAtIntersections) {
  ClipVertex a{{-2, 0, 0, 1}, {0, 0}, {0, 0, 0}};
  ClipVertex b{{0, -0.5f, 0, 1}, {1, 0}, {255, 0, 0}};
  ClipVertex c{{0, 0.5f, 0, 1}, {0, 1}, {0, 255, 0}};
  const auto polygon = ClipTriangle(a, b, c);
  ASSERT_EQ(polygon.size, 4u);
  int intersections = 0;
  for (size_t i = 0; i < polygon.size; ++i) {
    const auto& v = polygon.vertices[i];
    if (v.position.x != -1) {
      continue;
    }
    ++intersections;
    EXPECT_FLOAT_EQ(v.uv.x + v.uv.y, 0.5f);
    EXPECT_FLOAT_EQ(v.color.x + v.color.y, 127.5f);
  }
  EXPECT_EQ(intersections, 2);
}

TEST(Clipping, RejectsBehindCameraAndNonfiniteGeometry) {
  EXPECT_EQ(ClipTriangle(Vertex({-1, -1, 0, -1}), Vertex({1, -1, 0, -1}),
                         Vertex({0, 1, 0, -1}))
                .size,
            0u);
  EXPECT_EQ(
      ClipTriangle(Vertex({std::numeric_limits<float>::quiet_NaN(), 0, 0, 1}),
                   Vertex({1, -1, 0, 1}), Vertex({0, 1, 0, 1}))
          .size,
      0u);
}

TEST(Coverage, SharedDiagonalHasExactlyOneOwnerForEitherWinding) {
  for (bool reverse : {false, true}) {
    FrameBuffer first(4, 4), second(4, 4);
    first.Clear({});
    second.Clear({});
    Vertex2D a{0, 0, 0, kWhite}, b{4, 0, 0, kWhite}, c{4, 4, 0, kWhite},
        d{0, 4, 0, kWhite};
    first.DrawTriangle(a, reverse ? c : b, reverse ? b : c);
    second.DrawTriangle(a, reverse ? d : c, reverse ? c : d);
    for (int i = 0; i < 16; ++i) {
      EXPECT_EQ((first.Pixels()[i] != 0) + (second.Pixels()[i] != 0), 1)
          << "pixel " << i;
    }
  }
}

TEST(Coverage, ClippedSharedEdgesHaveNeitherGapsNorDoubleCoverage) {
  FrameBuffer first(8, 8), second(8, 8);
  first.Clear({});
  second.Clear({});
  const auto a = Vertex({-2, -1, 0, 1}), b = Vertex({1, -1, 0, 1});
  const auto c = Vertex({1, 1, 0, 1}), d = Vertex({-2, 1, 0, 1});
  DrawClipped(first, a, b, c);
  DrawClipped(second, a, c, d);
  for (int i = 0; i < 64; ++i) {
    EXPECT_EQ((first.Pixels()[i] != 0) + (second.Pixels()[i] != 0), 1)
        << "pixel " << i;
  }
}

TEST(Coverage, SubpixelDegeneratesAndNonfiniteTrianglesAreIgnored) {
  FrameBuffer frame(4, 4);
  frame.Clear({});
  Vertex2D a{1, 1, 0, kWhite}, b{1.00001f, 1, 0, kWhite},
      c{1, 1.00001f, 0, kWhite};
  frame.DrawTriangle(a, b, c);
  a.x = std::numeric_limits<float>::infinity();
  frame.DrawTriangle(a, b, c);
  EXPECT_EQ(ColoredPixels(frame), 0u);
}

TEST(Coverage, HorizontalAndVerticalSharedEdgesHaveExactlyOneOwner) {
  for (bool vertical : {false, true}) {
    FrameBuffer first(4, 4), second(4, 4);
    first.Clear({});
    second.Clear({});
    Vertex2D a{0, 0.5f, 0, kWhite}, b{4, 0.5f, 0, kWhite};
    Vertex2D c{2, -3.5f, 0, kWhite}, d{2, 4.5f, 0, kWhite};
    if (vertical) {
      std::swap(a.x, a.y);
      std::swap(b.x, b.y);
      std::swap(c.x, c.y);
      std::swap(d.x, d.y);
    }
    first.DrawTriangle(a, b, c);
    second.DrawTriangle(a, b, d);
    for (int i = 0; i < 4; ++i) {
      const int index = vertical ? i * 4 : i;
      EXPECT_EQ((first.Pixels()[index] != 0) + (second.Pixels()[index] != 0),
                1);
    }
  }
}

TEST(Clipping, HandlesCornersAndMultiplePlanesWithoutExceedingPolygonCapacity) {
  std::mt19937 random(42);
  std::uniform_int_distribution<int> coordinate(-4, 4);
  for (int iteration = 0; iteration < 3000; ++iteration) {
    const auto make_vertex = [&] {
      return Vertex({static_cast<float>(coordinate(random)),
                     static_cast<float>(coordinate(random)),
                     static_cast<float>(coordinate(random)),
                     static_cast<float>(coordinate(random))});
    };
    const auto a = make_vertex(), b = make_vertex(), c = make_vertex();
    const auto polygon = ClipTriangle(a, b, c);
    ASSERT_LE(polygon.size, 9u);
    for (size_t i = 0; i < polygon.size; ++i) {
      const auto p = polygon.vertices[i].position;
      EXPECT_LE(std::abs(p.x), p.w + 1e-5f);
      EXPECT_LE(std::abs(p.y), p.w + 1e-5f);
      EXPECT_LE(std::abs(p.z), p.w + 1e-5f);
    }
  }
}

TEST(Interpolation, ColorUsesReciprocalWButDepthIsLinearInScreenSpace) {
  FrameBuffer frame(4, 4);
  frame.Clear({});
  Vertex2D a{0, 0, 0, {255, 0, 0}, {}, 1};
  Vertex2D b{4, 0, 0.8f, {0, 255, 0}, {}, 0.5f};
  Vertex2D c{0, 4, 0.8f, {0, 0, 255}, {}, 0.25f};
  frame.DrawTriangle(a, b, c);
  EXPECT_EQ(frame.Pixels()[0], (227u << 16) | (19u << 8) | 9u);
  frame.BlendPixel(0, 0, 0.15f, {255, 255, 255});
  EXPECT_EQ(frame.Pixels()[0],
            0xffffffu);  // Projected depth was 0.2, not perspective-corrected.
}

TEST(Interpolation, TextureCoordinatesUseReciprocalW) {
  FrameBuffer frame(4, 4);
  frame.Clear({});
  Texture texture{2, 1, {{255, 0, 0}, {0, 255, 0}}};
  Vertex2D a{0, 0, 0, kWhite, {0, 0}, 1};
  Vertex2D b{4, 0, 0, kWhite, {1, 0}, 0.1f};
  Vertex2D c{0, 4, 0, kWhite, {0, 0}, 1};
  frame.DrawTriangle(a, b, c, &texture);
  EXPECT_EQ(frame.Pixels()[2], 0xff0000u);
}

TEST(Depth, IncludesNearAndFarPlanesAndFirstFragmentWinsTies) {
  FrameBuffer frame(1, 1);
  frame.Clear({});
  frame.BlendPixel(0, 0, 1, {255, 0, 0});
  EXPECT_EQ(frame.Pixels()[0], 0xff0000u);
  frame.BlendPixel(0, 0, 1, {0, 255, 0});
  EXPECT_EQ(frame.Pixels()[0], 0xff0000u);
  frame.BlendPixel(0, 0, -1, {0, 0, 255});
  EXPECT_EQ(frame.Pixels()[0], 255u);
}

TEST(Pipeline,
     HeadlessRenderingIsIdenticalAcrossWorkerCountsAndRepeatedFrames) {
  auto scene = TriangleScene();
  scene.vertices.push_back({-1, -1, 0, {255, 255, 255}});
  scene.triangles.push_back(
      {3, 1, 2});  // Equal-depth tie must remain in submission order.
  FrameBuffer single(65, 49), parallel(65, 49);
  SoftwareRenderer one(1), many(4);
  one.Render(scene, {}, Unlit(), single);
  ASSERT_GT(ColoredPixels(single), 0u);
  for (int frame = 0; frame < 3; ++frame) {
    many.Render(scene, {}, Unlit(), parallel);
    EXPECT_TRUE(std::equal(single.Pixels(), single.Pixels() + 65 * 49,
                           parallel.Pixels()));
  }
}

TEST(Pipeline, SupportsCullingModes) {
  const auto scene = TriangleScene();
  FrameBuffer frame(32, 32);
  SoftwareRenderer renderer(1);
  auto settings = Unlit();
  renderer.Render(scene, {}, settings, frame);
  EXPECT_GT(ColoredPixels(frame), 0u);
  settings.cull_mode = CullMode::kFront;
  renderer.Render(scene, {}, settings, frame);
  EXPECT_EQ(ColoredPixels(frame), 0u);
  settings.cull_mode = CullMode::kNone;
  renderer.Render(scene, {}, settings, frame);
  EXPECT_GT(ColoredPixels(frame), 0u);
}

TEST(Pipeline, RetainsVisiblePartWhenVertexIsBehindCamera) {
  auto scene = TriangleScene();
  scene.vertices[2].z = 6;
  auto settings = Unlit();
  settings.cull_mode = CullMode::kNone;
  FrameBuffer frame(32, 32);
  SoftwareRenderer renderer(2);
  renderer.Render(scene, {}, settings, frame);
  EXPECT_GT(ColoredPixels(frame), 0u);
}

TEST(Pipeline, HandlesEmptyScenesInvalidIndicesAndReuseAfterFailure) {
  FrameBuffer frame(16, 16);
  SoftwareRenderer renderer(2);
  renderer.Render(Scene{}, {}, Unlit(), frame);
  EXPECT_EQ(ColoredPixels(frame), 0u);
  auto scene = TriangleScene();
  scene.triangles.push_back({0, 1, 42});
  EXPECT_THROW(renderer.Render(scene, {}, Unlit(), frame),
               std::invalid_argument);
  scene.triangles.pop_back();
  EXPECT_NO_THROW(renderer.Render(scene, {}, Unlit(), frame));
  EXPECT_GT(ColoredPixels(frame), 0u);
}

TEST(CommandLine, ParsesHeadlessConfigurationAndRejectsBadValues) {
  EXPECT_EQ(Options({}).shading, ShadingMode::kPhong);
  EXPECT_EQ(Options({"--shading", "legacy"}).shading, ShadingMode::kLegacy);
  EXPECT_EQ(Options({"--filter", "bilinear"}).filter, TextureFilter::kBilinear);
  EXPECT_THROW(Options({"--shading", "oops"}), std::invalid_argument);
  EXPECT_THROW(Options({"--filter", "oops"}), std::invalid_argument);
  EXPECT_EQ(Options({"--tile-size", "32"}).tile_size, 32);
  EXPECT_THROW(Options({"--tile-size", "8"}), std::invalid_argument);
  const auto parsed =
      Options({"--headless", "--width", "64", "--height", "48", "--workers",
               "2", "--frames", "3", "--output", "frame.ppm"});
  EXPECT_TRUE(parsed.headless);
  EXPECT_EQ(parsed.width, 64);
  EXPECT_EQ(parsed.height, 48);
  EXPECT_EQ(parsed.workers, 2);
  EXPECT_EQ(parsed.frames, 3);
  EXPECT_EQ(parsed.output, "frame.ppm");
  EXPECT_EQ(Options({"--headless"}).frames, 1);
  EXPECT_TRUE(Options({"--model", "mesh.obj"}).texture.empty());
  EXPECT_THROW(Options({"--width", "0"}), std::invalid_argument);
  EXPECT_THROW(Options({"--width", "12px"}), std::invalid_argument);
  EXPECT_THROW(Options({"--workers", "-1"}), std::invalid_argument);
  EXPECT_THROW(Options({"--frames", "0"}), std::invalid_argument);
  EXPECT_THROW(Options({"--height"}), std::invalid_argument);
  EXPECT_THROW(Options({"--unknown"}), std::invalid_argument);
  EXPECT_THROW(Options({"--output", "frame.ppm"}), std::invalid_argument);
}

TEST(CommandLine, SelectsBuiltInScenesOrCustomModels) {
  const auto defaults = Options({});
  EXPECT_EQ(defaults.scene, ScenePreset::kPrimitives);
  EXPECT_TRUE(defaults.model.empty());
  EXPECT_EQ(Options({"--scene", "primitives"}).scene, ScenePreset::kPrimitives);
  const auto shrimp = Options({"--scene", "shrimp"});
  EXPECT_EQ(shrimp.scene, ScenePreset::kShrimp);
  const auto shrimp_scene = LoadScene(shrimp);
  ASSERT_EQ(shrimp_scene.objects.size(), 1u);
  EXPECT_FALSE(shrimp_scene.objects.front().Geometry().materials.empty());
  const auto custom = Options(
      {"--model", RENDERER_TEST_ASSET_DIR "/../tests/data/two_materials.obj"});
  EXPECT_EQ(custom.scene, ScenePreset::kModel);
  EXPECT_EQ(LoadScene(custom).objects.size(), 1u);
  EXPECT_THROW(Options({"--scene", "unknown"}), std::invalid_argument);
  EXPECT_THROW(Options({"--scene"}), std::invalid_argument);
  EXPECT_THROW(Options({"--model", ""}), std::invalid_argument);
  EXPECT_THROW(Options({"--scene", "primitives", "--model", "mesh.obj"}),
               std::invalid_argument);
  EXPECT_THROW(Options({"--model", "mesh.obj", "--scene", "shrimp"}),
               std::invalid_argument);
  EXPECT_THROW(Options({"--texture", "image.png"}), std::invalid_argument);
  EXPECT_EQ(Options({"--scene", "shrimp", "--texture", "image.png"}).texture,
            "image.png");
}
