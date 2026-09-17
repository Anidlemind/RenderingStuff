#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <sstream>

#include "app/options.h"
#include "assets/mtl_loader.h"
#include "assets/obj_loader.h"
#include "render/color_space.h"
#include "render/software_renderer.h"
#include "ui/text.h"

namespace {
std::shared_ptr<Mesh> QuadMesh() {
  auto mesh = std::make_shared<Mesh>();
  mesh->vertices = {{-1, -1, 0, {255, 255, 255}, {0, 0, 1}},
                    {1, -1, 0, {255, 255, 255}, {0, 0, 1}},
                    {1, 1, 0, {255, 255, 255}, {0, 0, 1}},
                    {-1, 1, 0, {255, 255, 255}, {0, 0, 1}}};
  mesh->triangles = {{0, 1, 2}, {0, 2, 3}};
  return mesh;
}
RenderSettings UnlitSettings() {
  RenderSettings settings;
  settings.shading = ShadingMode::kUnlit;
  settings.clear_color = {};
  return settings;
}
size_t NonblackPixels(const FrameBuffer& frame) {
  return std::count_if(frame.Pixels(),
                       frame.Pixels() + frame.Width() * frame.Height(),
                       [](uint32_t pixel) { return pixel != 0; });
}
}  // namespace

TEST(Normalization, HandlesZeroTinyAndHugeFiniteVectors) {
  EXPECT_FLOAT_EQ(Normalize(Vec3{}).Length(), 0);
  EXPECT_FLOAT_EQ(Normalize(Vec2{}).Length(), 0);
  EXPECT_FLOAT_EQ(Normalize(Vec4{}).Length(), 0);
  EXPECT_NEAR(Normalize(Vec3{1e30f, 1e30f, 1e30f}).Length(), 1, 1e-6f);
  EXPECT_NEAR(Normalize(Vec3{1e-35f, 1e-35f, 1e-35f}).Length(), 1, 1e-6f);
  EXPECT_FLOAT_EQ(
      Normalize(Vec3{std::numeric_limits<float>::infinity(), 0, 0}).Length(),
      0);
}

TEST(FrameBuffer, ClipsExtremeLineEndpointsBeforeRasterization) {
  FrameBuffer frame(4, 4);
  frame.DrawLine(std::numeric_limits<int>::min(), 2,
                 std::numeric_limits<int>::max(), 2, {255, 255, 255});
  EXPECT_EQ(NonblackPixels(frame), 4u);
  frame.DrawLine(std::numeric_limits<int>::min(), -2,
                 std::numeric_limits<int>::max(), -2, {255, 255, 255});
  EXPECT_EQ(NonblackPixels(frame), 4u);
}

TEST(ObjNormals, GeneratesFlatNormalsAndPreservesExplicitNormals) {
  std::istringstream input(
      "v 0 0 0\nv 1 0 0\nv 0 1 0\nv 0 0 1\nvn 0 0 -1\nf 1 2 3\nf 1 4 2\nf 1//1 "
      "2//1 3//1\n");
  const auto mesh = LoadObj(input);
  ASSERT_EQ(mesh.vertices.size(), 9u);
  EXPECT_FLOAT_EQ(mesh.vertices[0].normal.z, 1);
  EXPECT_FLOAT_EQ(mesh.vertices[3].normal.y, 1);
  EXPECT_FLOAT_EQ(mesh.vertices[6].normal.z, -1);
}

TEST(ObjNormals, ExtremePositionsNormalizeWithoutOverflow) {
  const auto mesh =
      Mesh::LoadFromObj(RENDERER_TEST_ASSET_DIR "/../tests/data/extreme.obj");
  ASSERT_EQ(mesh.vertices.size(), 3u);
  for (const auto& vertex : mesh.vertices) {
    EXPECT_TRUE(std::isfinite(vertex.x));
    EXPECT_LE(std::abs(vertex.x), 1);
    EXPECT_LE(std::abs(vertex.y), 1);
    EXPECT_FLOAT_EQ(vertex.normal.z, 1);
  }
  std::istringstream tiny("v 0 0 0\nv 1e-30 0 0\nv 0 1e-30 0\nf 1 2 3\n");
  EXPECT_FLOAT_EQ(LoadObj(tiny).vertices[0].normal.z, 1);
}

TEST(ObjMaterials, ReportsMissingFilesAndMalformedMtlLocation) {
  EXPECT_THROW(LoadMtl(RENDERER_TEST_ASSET_DIR "/missing.mtl"),
               std::runtime_error);
  try {
    LoadMtl(RENDERER_TEST_ASSET_DIR "/../tests/data/materials/invalid.mtl");
    FAIL() << "Invalid Ns accepted";
  } catch (const std::runtime_error& error) {
    EXPECT_NE(std::string(error.what()).find("invalid.mtl: line 2"),
              std::string::npos);
  }
}

TEST(TextOverlay, SupportsSceneLabelsAndClipsExtremeCoordinates) {
  FrameBuffer frame(64, 16);
  DrawText(frame, std::numeric_limits<int>::max(),
           std::numeric_limits<int>::max(), "FPS", {255, 255, 255});
  DrawText(frame, std::numeric_limits<int>::min(),
           std::numeric_limits<int>::min(), "FPS", {255, 255, 255});
  EXPECT_EQ(NonblackPixels(frame), 0u);
  for (char letter : std::string("OBJ/CUL")) {
    frame.Clear({});
    DrawText(frame, 0, 0, std::string(1, letter), {255, 255, 255});
    EXPECT_GT(NonblackPixels(frame), 0u) << letter;
  }
}

TEST(ObjNormals, SmoothsAcrossUvSeamsButNotAcrossSmoothingGroups) {
  std::istringstream input(
      "v 0 0 0\nv 1 0 0\nv 0 1 0\nv 0 0 1\nvt 0 0\nvt 1 1\ns 1\nf 1/1 2/1 "
      "3/1\nf 1/2 4/2 2/2\ns 2\nf 1 2 3\n");
  const auto mesh = LoadObj(input);
  ASSERT_EQ(mesh.vertices.size(), 9u);
  EXPECT_NEAR(mesh.vertices[0].normal.y, std::sqrt(0.5f), 1e-6f);
  EXPECT_FLOAT_EQ(mesh.vertices[0].normal.y, mesh.vertices[3].normal.y);
  EXPECT_FLOAT_EQ(mesh.vertices[6].normal.y, 0);
  EXPECT_FLOAT_EQ(mesh.vertices[6].normal.z, 1);
}

TEST(ObjMaterials, LoadsPerFacePropertiesAndResolvesTextureRelativeToMtl) {
  const auto mesh = Mesh::LoadFromObj(RENDERER_TEST_ASSET_DIR
                                      "/../tests/data/two_materials.obj");
  ASSERT_EQ(mesh.materials.size(), 2u);
  ASSERT_EQ(mesh.triangle_materials, (std::vector<int>{0, 1}));
  EXPECT_EQ(mesh.materials[0].surface.diffuse.r, 255);
  EXPECT_EQ(mesh.materials[0].surface.diffuse.g, 0);
  EXPECT_FLOAT_EQ(mesh.materials[0].surface.shininess, 1);
  EXPECT_EQ(mesh.materials[1].surface.diffuse.r, 188);
  ASSERT_TRUE(mesh.materials[1].texture);
  EXPECT_FALSE(mesh.materials[1].texture->mipmaps.empty());
}

TEST(SceneInstances, ShareGeometryWithIndependentTransformsAndMaterials) {
  auto mesh = QuadMesh();
  Scene scene;
  scene.objects.emplace_back(mesh);
  scene.objects.emplace_back(mesh);
  scene.objects[0].model = Translation({-1.2f, 0, 0}) * Scale(0.7f);
  scene.objects[1].model = Translation({1.2f, 0, 0}) * Scale(0.7f);
  scene.objects[0].material = Material{{255, 0, 0}, {}, 1};
  scene.objects[1].material = Material{{0, 255, 0}, {}, 1};
  EXPECT_EQ(&scene.objects[0].Geometry(), &scene.objects[1].Geometry());
  FrameBuffer frame(96, 64);
  SoftwareRenderer renderer(2);
  auto stats = renderer.Render(scene, {}, UnlitSettings(), frame);
  EXPECT_EQ(stats.input_objects, 2u);
  EXPECT_EQ(stats.input_triangles, 4u);
  EXPECT_EQ(frame.Pixels()[32 * 96 + 34], 0xff0000u);
  EXPECT_EQ(frame.Pixels()[32 * 96 + 62], 0x00ff00u);
}

TEST(SceneInstances, DepthAndEqualDepthTieAreStableAcrossObjects) {
  auto mesh = QuadMesh();
  Scene scene;
  scene.objects.emplace_back(mesh);
  scene.objects.emplace_back(mesh);
  scene.objects[0].material = Material{{255, 0, 0}, {}, 1};
  scene.objects[1].material = Material{{0, 255, 0}, {}, 1};
  FrameBuffer frame(32, 32);
  SoftwareRenderer renderer(3);
  renderer.Render(scene, {}, UnlitSettings(), frame);
  EXPECT_EQ(frame.Pixels()[16 * 32 + 16], 0xff0000u);
  scene.objects[1].model = Translation({0, 0, 1});
  renderer.Render(scene, {}, UnlitSettings(), frame);
  EXPECT_EQ(frame.Pixels()[16 * 32 + 16], 0x00ff00u);
  std::reverse(scene.objects.begin(), scene.objects.end());
  renderer.Render(scene, {}, UnlitSettings(), frame);
  EXPECT_EQ(frame.Pixels()[16 * 32 + 16], 0x00ff00u);
}

TEST(SceneCulling, MatchesUnculledImageAndRejectsAllSixOutsidePlanes) {
  auto mesh = QuadMesh();
  Scene scene;
  scene.objects.emplace_back(mesh);
  for (const Vec3 offset : {Vec3{100, 0, 0},
                            {-100, 0, 0},
                            {0, 100, 0},
                            {0, -100, 0},
                            {0, 0, 100},
                            {0, 0, -200}}) {
    scene.objects.emplace_back(mesh);
    scene.objects.back().model = Translation(offset);
  }
  FrameBuffer culled(65, 49), full(65, 49);
  SoftwareRenderer renderer(2);
  auto settings = UnlitSettings();
  const auto stats = renderer.Render(scene, {}, settings, culled);
  EXPECT_EQ(stats.culled_objects, 6u);
  EXPECT_EQ(stats.input_triangles, 14u);
  settings.object_culling = false;
  renderer.Render(scene, {}, settings, full);
  EXPECT_TRUE(
      std::equal(culled.Pixels(), culled.Pixels() + 65 * 49, full.Pixels()));
  EXPECT_TRUE(IsVisible({{-2, -2, -2}, {2, 2, 2}}, Mat4::Identity()));
  EXPECT_TRUE(IsVisible({{-1, -1, -1}, {1, 1, 1}}, Mat4::Identity()));
}

TEST(SceneValidation, RejectsNullMeshesAndInvalidMaterialMappings) {
  EXPECT_THROW(MeshInstance(nullptr), std::invalid_argument);
  auto mesh = QuadMesh();
  mesh->triangle_materials = {0};
  Scene scene;
  scene.objects.emplace_back(mesh);
  FrameBuffer frame(8, 8);
  SoftwareRenderer renderer(1);
  EXPECT_THROW(renderer.Render(scene, {}, {}, frame), std::invalid_argument);
  mesh->triangle_materials = {0, 0};
  EXPECT_THROW(renderer.Render(scene, {}, {}, frame), std::invalid_argument);
}

TEST(MaterialRendering, UsesTriangleMaterialsAndInstanceOverride) {
  auto mesh = QuadMesh();
  mesh->materials = {{"red", {{255, 0, 0}, {}, 1}, nullptr},
                     {"green", {{0, 255, 0}, {}, 1}, nullptr}};
  mesh->triangle_materials = {0, 1};
  Scene scene;
  scene.objects.emplace_back(mesh);
  FrameBuffer frame(64, 64);
  SoftwareRenderer renderer(2);
  renderer.Render(scene, {}, UnlitSettings(), frame);
  EXPECT_GT(std::count(frame.Pixels(), frame.Pixels() + 4096, 0xff0000u), 0);
  EXPECT_GT(std::count(frame.Pixels(), frame.Pixels() + 4096, 0x00ff00u), 0);
  scene.objects[0].material = Material{{0, 0, 255}, {}, 1};
  renderer.Render(scene, {}, UnlitSettings(), frame);
  EXPECT_EQ(std::count(frame.Pixels(), frame.Pixels() + 4096, 0xff0000u), 0);
  EXPECT_GT(std::count(frame.Pixels(), frame.Pixels() + 4096, 0x0000ffu), 0);
}

TEST(DebugViews, NormalsDepthWireframeAndOverdrawHaveDefinedMeaning) {
  auto mesh = QuadMesh();
  Scene scene;
  scene.objects.emplace_back(mesh);
  FrameBuffer frame(64, 64);
  SoftwareRenderer renderer(2);
  auto settings = UnlitSettings();
  settings.debug_view = DebugView::kNormals;
  renderer.Render(scene, {}, settings, frame);
  EXPECT_EQ(frame.Pixels()[32 * 64 + 32], 0x8080ffu);
  const auto filled = NonblackPixels(frame);
  settings.debug_view = DebugView::kDepth;
  renderer.Render(scene, {}, settings, frame);
  const auto depth_color = frame.Pixels()[32 * 64 + 32];
  EXPECT_GT(depth_color, 0u);
  EXPECT_EQ(depth_color & 255, (depth_color >> 8) & 255);
  settings.debug_view = DebugView::kWireframe;
  renderer.Render(scene, {}, settings, frame);
  EXPECT_GT(NonblackPixels(frame), 0u);
  EXPECT_LT(NonblackPixels(frame), filled);
  scene.objects.emplace_back(mesh);
  settings.debug_view = DebugView::kOverdraw;
  renderer.Render(scene, {}, settings, frame);
  EXPECT_EQ(frame.Pixels()[32 * 64 + 32],
            0x40d060u);  // Two surfaces, no shared-edge double count.
}

TEST(Antialiasing, ResolveAveragesCoverageInLinearLight) {
  FrameBuffer source(2, 2), target(1, 1);
  source.SetPixel(0, 0, {255, 255, 255});
  source.SetPixel(1, 1, {255, 255, 255});
  target.ResolveSupersampling(source);
  EXPECT_EQ(target.Pixels()[0], 0xbcbcbcu);
  EXPECT_TRUE(std::isinf(target.Depth()[0]));
  EXPECT_THROW(source.ResolveSupersampling(target), std::invalid_argument);
}

TEST(Antialiasing, RowRangesMatchLinearReferenceAndLeaveOtherRowsUntouched) {
  constexpr int kWidth = 257, kHeight = 5;
  FrameBuffer source(kWidth * 2, kHeight * 2), target(kWidth, kHeight);
  for (int y = 0; y < kHeight * 2; ++y) {
    for (int x = 0; x < kWidth * 2; ++x) {
      const auto value = static_cast<uint8_t>(x / 2);
      source.SetPixel(x, y,
                      y < 2 ? Color{value, value, value}
                            : Color{static_cast<uint8_t>(x * 37 + y * 17),
                                    static_cast<uint8_t>(x * 13 + y * 31),
                                    static_cast<uint8_t>(x * 7 + y * 43)});
    }
  }
  target.Clear({1, 2, 3});
  target.BlendPixel(0, 0, 0, {1, 2, 3});
  target.ResolveSupersamplingRange(source, 4, 2);
  target.ResolveSupersamplingRange(source, kHeight, kHeight + 2);
  target.ResolveSupersamplingRange(source, -3, -1);
  target.ResolveSupersamplingRange(source, 2, kHeight + 3);
  EXPECT_EQ(target.Pixels()[0], 0x010203u);
  EXPECT_FLOAT_EQ(target.Depth()[0], 0);
  target.ResolveSupersamplingRange(source, -3, 1);
  const auto linear = [](uint32_t pixel) {
    return ToLinear({static_cast<uint8_t>(pixel >> 16),
                     static_cast<uint8_t>(pixel >> 8),
                     static_cast<uint8_t>(pixel)});
  };
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const int i = y * 2 * source.Width() + x * 2;
      const auto* pixels = source.Pixels();
      const auto expected = ToSrgb((linear(pixels[i]) + linear(pixels[i + 1]) +
                                    linear(pixels[i + source.Width()]) +
                                    linear(pixels[i + source.Width() + 1])) *
                                   0.25f);
      const uint32_t packed = (static_cast<uint32_t>(expected.r) << 16) |
                              (static_cast<uint32_t>(expected.g) << 8) |
                              expected.b;
      EXPECT_EQ(target.Pixels()[y * kWidth + x], packed);
      EXPECT_TRUE(std::isinf(target.Depth()[y * kWidth + x]));
    }
  }
  EXPECT_THROW(source.ResolveSupersamplingRange(target, 0, 0),
               std::invalid_argument);
}

TEST(Antialiasing, ProducesPartialEdgePixelsAndReusesBuffersAcrossSizes) {
  auto mesh = QuadMesh();
  mesh->triangles.resize(1);
  SoftwareRenderer renderer(2);
  auto settings = UnlitSettings();
  settings.antialiasing = Antialiasing::kSsaa4;
  for (int size : {31, 64, 31}) {
    FrameBuffer target(size, size);
    renderer.Render(*mesh, {}, settings, target);
    EXPECT_GT(std::count_if(target.Pixels(), target.Pixels() + size * size,
                            [](uint32_t p) { return p != 0 && p != 0xffffff; }),
              0);
  }
}

TEST(ShadowMap, PcfDepthComparisonBiasAndAmbientAreIndependent) {
  ShadowMap map;
  map.size = 4;
  map.depth.assign(16, 0);
  map.bias = 0;
  EXPECT_FLOAT_EQ(map.Visibility({0, 0, 0.5f}, {0, 0, 1}, {0, 0, 1}), 0);
  EXPECT_FLOAT_EQ(map.Visibility({0, 0, -0.5f}, {0, 0, 1}, {0, 0, 1}), 1);
  EXPECT_FLOAT_EQ(map.Visibility({2, 0, 0.5f}, {0, 0, 1}, {0, 0, 1}), 1);
  FragmentSettings settings;
  settings.shadow_map = &map;
  settings.light.direction = {0, 0, 1};
  settings.light.ambient = 0.2f;
  settings.camera_position = {0, 0, 5};
  EXPECT_NEAR(ShadeBlinnPhong({1, 1, 1}, {0, 0, 1}, {0, 0, 0.5f}, settings).x,
              0.2f, 1e-6f);
  map.bias = 0.6f;
  EXPECT_FLOAT_EQ(map.Visibility({0, 0, 0.5f}, {0, 0, 1}, {0, 0, 1}), 1);
}

TEST(ShadowMap, SlopedReceiverStaysLitButStillReceivesOtherShadows) {
  for (int size : {32, 128, 512}) {
    ShadowMap map;
    map.size = size;
    map.bias = 0.00001f;
    map.depth.resize(size * size);
    const auto plane = [](float x, float y) { return 0.4f * x - 0.3f * y; };
    for (int y = 0; y < size; ++y) {
      for (int x = 0; x < size; ++x) {
        map.depth[y * size + x] =
            plane(2 * (x + 0.5f) / size - 1, 1 - 2 * (y + 0.5f) / size);
      }
    }
    const auto gradient = map.ReceiverDepthGradient(
        {-1, -1, plane(-1, -1)}, {1, -1, plane(1, -1)}, {-1, 1, plane(-1, 1)});
    EXPECT_NEAR(gradient.x, 0.8f / size, 1e-7f);
    EXPECT_NEAR(gradient.y, 0.6f / size, 1e-7f);
    const Vec3 normal{-0.4f, 0.3f, 1};
    for (float x : {-0.23f, 0.0f, 0.17f}) {
      for (float y : {-0.31f, 0.03f, 0.26f}) {
        const Vec3 position{x, y, plane(x, y)};
        EXPECT_LT(map.Visibility(position, normal, {0, 0, 1}), 0.8f);
        EXPECT_NEAR(map.Visibility(position, normal, {0, 0, 1}, gradient), 1,
                    1e-6f);
      }
    }
    for (auto& depth : map.depth) {
      depth -= 0.1f;
    }
    EXPECT_FLOAT_EQ(map.Visibility({}, normal, {0, 0, 1}, gradient), 0);
  }
}

TEST(ShadowMap, ReceiverGradientRespectsProjectionAndDegenerateFaces) {
  ShadowMap map;
  map.size = 64;
  map.view_projection =
      Translation({0.1f, 0.2f, 0.3f}) * Scale({0.5f, 0.25f, 2});
  const Vec3 a{0, 0, 0}, b{1, 0, 0.5f}, c{0, 1, -0.25f};
  for (const auto gradient : {map.ReceiverDepthGradient(a, b, c),
                              map.ReceiverDepthGradient(a, c, b)}) {
    EXPECT_NEAR(gradient.x, 4.0f / map.size, 1e-7f);
    EXPECT_NEAR(gradient.y, 4.0f / map.size, 1e-7f);
  }
  for (const auto gradient : {map.ReceiverDepthGradient(a, a, a),
                              map.ReceiverDepthGradient(a, {0, 0, 1}, c)}) {
    EXPECT_FLOAT_EQ(gradient.x, 0);
    EXPECT_FLOAT_EQ(gradient.y, 0);
  }
  map.size = 0;
  EXPECT_FLOAT_EQ(map.ReceiverDepthGradient(a, b, c).Length(), 0);
}

TEST(ShadowRendering, SmoothSphereHasNoSelfShadowGridOnItsLitSurface) {
  Scene scene;
  scene.objects.emplace_back(std::make_shared<Mesh>(Mesh::MakeSphere()));
  Camera camera;
  camera.position = {0, 0, 3};
  camera.fov_y = Radians(60);
  RenderSettings settings;
  settings.shading = ShadingMode::kPhong;
  settings.ambient = 0;
  settings.clear_color = {};
  settings.material.specular = {};
  SoftwareRenderer renderer(2);
  FrameBuffer reference(160, 160), shadowed(160, 160);
  renderer.Render(scene, camera, settings, reference);
  settings.shadows = true;
  for (int size : {128, 512, 1024}) {
    settings.shadow_size = size;
    renderer.Render(scene, camera, settings, shadowed);
    int compared = 0, maximum_error = 0;
    for (int i = 0; i < 160 * 160; ++i) {
      const int expected = reference.Pixels()[i] & 255;
      // Compare the lit hemisphere, away from the mesh's light terminator.
      if (expected >= 128) {
        ++compared;
        maximum_error = std::max(
            maximum_error,
            std::abs(expected - static_cast<int>(shadowed.Pixels()[i] & 255)));
      }
    }
    EXPECT_GT(compared, 1000);
    EXPECT_LE(maximum_error, 1) << "shadow resolution=" << size;
  }
}

TEST(ShadowRendering, OffCameraCasterStillShadowsVisibleReceiver) {
  auto quad = QuadMesh();
  Scene scene;
  scene.objects.emplace_back(quad);
  scene.objects[0].casts_shadow = false;
  scene.objects.emplace_back(quad);
  scene.objects[1].model = Translation({3, 0, 3}) * Scale(0.7f);
  Camera camera;
  camera.fov_y = Radians(40);
  auto settings = UnlitSettings();
  settings.shading = ShadingMode::kPhong;
  settings.light_direction = {1, 0, 1};
  settings.material.specular = {};
  settings.shadow_size = 128;
  SoftwareRenderer renderer(2);
  FrameBuffer lit(64, 64), shadowed(64, 64);
  renderer.Render(scene, camera, settings, lit);
  settings.shadows = true;
  auto stats = renderer.Render(scene, camera, settings, shadowed);
  EXPECT_EQ(stats.culled_objects, 1u);
  EXPECT_GT(stats.shadow_ms, 0);
  EXPECT_LT(shadowed.Pixels()[32 * 64 + 32] & 255,
            lit.Pixels()[32 * 64 + 32] & 255);
  scene.objects[1].casts_shadow = false;
  renderer.Render(scene, camera, settings, shadowed);
  EXPECT_TRUE(std::equal(lit.Pixels(), lit.Pixels() + 4096, shadowed.Pixels()));
  scene.objects[1].casts_shadow = true;
  scene.objects[0].receives_shadow = false;
  renderer.Render(scene, camera, settings, shadowed);
  EXPECT_TRUE(std::equal(lit.Pixels(), lit.Pixels() + 4096, shadowed.Pixels()));
}

TEST(ShadowMap, VisibilityIsContinuousAcrossTexelBoundaries) {
  ShadowMap map;
  map.size = 8;
  map.bias = 0;
  map.depth.assign(64, std::numeric_limits<float>::infinity());
  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 4; ++x) {
      map.depth[y * 8 + x] = 0;
    }
  }
  const auto visibility = [&](float x) {
    return map.Visibility({x, 0, 0.5f}, {0, 0, 1}, {0, 0, 1});
  };
  EXPECT_NEAR(visibility(0), 0.5f, 1e-6f);
  EXPECT_NEAR(visibility(-0.0001f), visibility(0.0001f), 0.001f);
  EXPECT_NEAR(visibility(-0.125f), 1.0f / 3, 1e-6f);
  EXPECT_NEAR(visibility(0.125f), 2.0f / 3, 1e-6f);
  float previous = visibility(-0.5f);
  for (int i = 1; i <= 100; ++i) {
    const float current = visibility(-0.5f + i * 0.01f);
    EXPECT_GE(current, previous);
    EXPECT_GE(current, 0);
    EXPECT_LE(current, 1);
    previous = current;
  }
}

TEST(ShadowMap, FilterInterpolatesComparisonsInsteadOfDepths) {
  ShadowMap map;
  map.size = 8;
  map.bias = 0;
  map.depth.resize(64);
  for (int i = 0; i < 64; ++i) {
    map.depth[i] = (i + i / 8) % 2 ? 0.8f : -0.8f;
  }
  // Half the opaque texels occlude the receiver. Filtering depths first would
  // invent an intermediate surface and turn a partially lit pixel fully dark.
  for (float receiver : {-0.3f, 0.3f}) {
    EXPECT_NEAR(map.Visibility({0, 0, receiver}, {0, 0, 1}, {0, 0, 1}), 0.5f,
                1e-6f);
  }
}

TEST(ShadowProjection, LargeReceiverDoesNotDiluteCasterResolution) {
  auto mesh = QuadMesh();
  Scene scene;
  scene.objects.emplace_back(mesh);
  const auto original = ShadowViewProjection(scene, {0, 0, 1});
  scene.objects.emplace_back(mesh);
  scene.objects.back().casts_shadow = false;
  scene.objects.back().model = Translation({0, 0, -10}) * Scale(100);
  const auto with_floor = ShadowViewProjection(scene, {0, 0, 1});
  const auto caster_corner = Vec4{1, 1, 0, 1};
  const auto before = original * caster_corner;
  const auto after = with_floor * caster_corner;
  EXPECT_FLOAT_EQ(before.x, after.x);
  EXPECT_FLOAT_EQ(before.y, after.y);
  EXPECT_GT(after.x, 0.9f);
  EXPECT_LT(after.x, 1);
  const auto receiver = with_floor * Vec4{0, 0, -10, 1};
  EXPECT_GE(receiver.z, -1);
  EXPECT_LE(receiver.z, 1);
  EXPECT_LT(after.z, receiver.z);
}

TEST(ShadowProjection, FitsRotatedCastersAndHandlesEmptyScenes) {
  Scene scene;
  scene.objects.emplace_back(QuadMesh());
  scene.objects[0].model =
      Translation({4, 2, -3}) * RotationY(0.7f) * Scale({2, 3, 1});
  for (const Vec3 direction : {Vec3{0, 1, 0}, {1, 2, 3}, {0, -1, 0}}) {
    const auto projection = ShadowViewProjection(scene, direction);
    for (const auto& vertex : scene.objects[0].Geometry().vertices) {
      const auto p = projection * scene.objects[0].model *
                     Vec4{vertex.x, vertex.y, vertex.z, 1};
      EXPECT_LE(std::abs(p.x), 1);
      EXPECT_LE(std::abs(p.y), 1);
      EXPECT_LE(std::abs(p.z), 1);
    }
  }
  EXPECT_NO_THROW(ShadowViewProjection(Scene{}, {0, 1, 0}));
  EXPECT_THROW(ShadowViewProjection(scene, {}), std::invalid_argument);
}

TEST(PrimitiveMeshes, HaveOutwardFacesAndUnitNormals) {
  for (const auto& mesh :
       {Mesh::MakeCube(), Mesh::MakeSphere(), Mesh::MakePyramid()}) {
    for (const auto& vertex : mesh.vertices) {
      EXPECT_NEAR(vertex.normal.Length(), 1, 1e-5f);
    }
    for (const auto& triangle : mesh.triangles) {
      const auto position = [&](int index) {
        const auto& vertex = mesh.vertices.at(index);
        return Vec3{vertex.x, vertex.y, vertex.z};
      };
      const auto a = position(triangle[0]);
      const auto b = position(triangle[1]);
      const auto c = position(triangle[2]);
      const auto normal = Cross(b - a, c - a);
      EXPECT_GT(Dot(normal, a + b + c), 0);
      for (int index : triangle) {
        EXPECT_GT(Dot(normal, mesh.vertices[index].normal), 0);
      }
    }
  }
}

TEST(PrimitiveScene, ShapesRestOnTheFloorAndAreVisible) {
  const auto scene = LoadScene(AppOptions{});
  ASSERT_EQ(scene.objects.size(), 4u);
  const auto& floor = scene.objects.back();
  const float floor_y = floor.Bounds().maximum.y;
  EXPECT_FLOAT_EQ(floor.Bounds().minimum.y, floor_y);
  const auto camera = MakeCamera(AppOptions{});
  SoftwareRenderer renderer(1);
  auto settings = UnlitSettings();
  for (size_t i = 0; i + 1 < scene.objects.size(); ++i) {
    Scene isolated;
    isolated.objects.push_back(scene.objects[i]);
    EXPECT_NEAR(WorldBounds(isolated).minimum.y, floor_y, 1e-5f);
    for (const auto& vertex : isolated.objects.front().Geometry().vertices) {
      const auto p = camera.ViewProjection(4.0f / 3) *
                     isolated.objects.front().model *
                     Vec4{vertex.x, vertex.y, vertex.z, 1};
      EXPECT_LT(std::abs(p.x), p.w);
      EXPECT_LT(std::abs(p.y), p.w);
      EXPECT_LT(std::abs(p.z), p.w);
    }
    FrameBuffer frame(96, 72);
    renderer.Render(isolated, camera, settings, frame);
    EXPECT_GT(NonblackPixels(frame), 40u);
  }
}

TEST(ScenePipeline, AllModesAreDeterministicAcrossTilesAndWorkers) {
  const auto scene = MakePrimitivesScene();
  const auto camera = MakePrimitivesCamera();
  SoftwareRenderer serial(1), parallel(4);
  FrameBuffer reference(65, 49), actual(65, 49);
  auto settings = UnlitSettings();
  settings.shading = ShadingMode::kPhong;
  settings.shadows = true;
  settings.shadow_size = 64;
  settings.antialiasing = Antialiasing::kSsaa4;
  for (auto view : {DebugView::kNone, DebugView::kDepth, DebugView::kNormals,
                    DebugView::kWireframe, DebugView::kOverdraw}) {
    settings.debug_view = view;
    settings.tile_size = 0;
    serial.Render(scene, camera, settings, reference);
    EXPECT_GT(NonblackPixels(reference), 0u);
    for (int tile : {0, 16, 32}) {
      settings.tile_size = tile;
      parallel.Render(scene, camera, settings, actual);
      EXPECT_TRUE(std::equal(reference.Pixels(), reference.Pixels() + 65 * 49,
                             actual.Pixels()))
          << static_cast<int>(view) << " tile=" << tile;
    }
  }
}

TEST(SceneOptions, ParsesModesAndRejectsInvalidValues) {
  const auto parse = [](std::vector<std::string> values) {
    std::vector<char*> args;
    for (auto& value : values) {
      args.push_back(value.data());
    }
    return ParseOptions(static_cast<int>(args.size()), args.data(), true);
  };
  const auto options =
      parse({"renderer", "--scene", "primitives", "--shadows", "--shadow-size",
             "128", "--debug", "normals", "--aa", "ssaa4", "--no-texture"});
  EXPECT_EQ(options.scene, ScenePreset::kPrimitives);
  EXPECT_TRUE(options.shadows);
  EXPECT_TRUE(options.no_texture);
  EXPECT_EQ(options.debug_view, DebugView::kNormals);
  EXPECT_EQ(options.antialiasing, Antialiasing::kSsaa4);
  EXPECT_THROW(parse({"renderer", "--aa", "unknown"}), std::invalid_argument);
  EXPECT_THROW(parse({"renderer", "--debug", "unknown"}),
               std::invalid_argument);
  EXPECT_THROW(parse({"renderer", "--shadow-size", "0"}),
               std::invalid_argument);
  EXPECT_THROW(parse({"renderer", "--aa", "ssaa4", "--width", "8193"}),
               std::invalid_argument);
}
