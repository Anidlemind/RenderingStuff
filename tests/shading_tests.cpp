#include <gtest/gtest.h>

#include "render/color_space.h"
#include "render/shading.h"
#include "render/software_renderer.h"
#include "scene/benchmark_scenes.h"

TEST(ColorSpace, RoundTripsBytesAndEncodesLinearHalfAs188) {
  for (int i = 0; i < 256; ++i) {
    const auto value = static_cast<uint8_t>(i);
    EXPECT_EQ(ToSrgb(ToLinear({value, value, value})).r, value);
  }
  EXPECT_EQ(ToSrgb({0.5f, 0.5f, 0.5f}).r, 188);
  EXPECT_NEAR(SrgbToLinear(128), 0.21586f, 1e-5f);
}

TEST(TextureFiltering, BilinearUsesLinearLightAndWrapsAtTexelCenters) {
  Texture texture{2, 1, {{0, 0, 0}, {255, 255, 255}}};
  EXPECT_FLOAT_EQ(texture.SampleLinear(0.25f, 0.5f, TextureFilter::kBilinear).x,
                  0);
  EXPECT_FLOAT_EQ(texture.SampleLinear(0.75f, 0.5f, TextureFilter::kBilinear).x,
                  1);
  EXPECT_FLOAT_EQ(texture.SampleLinear(0.5f, 0.5f, TextureFilter::kBilinear).x,
                  0.5f);
  EXPECT_FLOAT_EQ(texture.SampleLinear(0, 0.5f, TextureFilter::kBilinear).x,
                  0.5f);
  EXPECT_FLOAT_EQ(
      texture.SampleLinear(-0.25f, 0.5f, TextureFilter::kBilinear).x, 1);
  texture.GenerateMipmaps();
  EXPECT_FLOAT_EQ(texture.SampleLinear(0.5f, 0.5f, TextureFilter::kBilinear).x,
                  0.5f);
  EXPECT_EQ(texture.mipmaps.size(), 2u);
}

TEST(TextureFiltering, MipmapsIncludeOddEdgesAndHandleOneDimensionalTextures) {
  Texture texture{3, 5, std::vector<Color>(15, {0, 0, 0})};
  for (int y = 0; y < 5; ++y) {
    texture.pixels[y * 3 + 2] = {255, 255, 255};
  }
  texture.GenerateMipmaps();
  ASSERT_EQ(texture.mipmaps.size(), 3u);
  EXPECT_EQ(texture.mipmaps.back().width, 1);
  EXPECT_EQ(texture.mipmaps.back().height, 1);
  EXPECT_NEAR(texture.mipmaps.back().pixels[0].x, 1.0f / 3, 1e-6f);
  texture.pixels.assign(15, {255, 255, 255});
  texture.GenerateMipmaps();
  EXPECT_FLOAT_EQ(texture.mipmaps.back().pixels[0].x, 1);
  Texture vertical{1, 3, {{0, 0, 0}, {255, 255, 255}, {0, 0, 0}}};
  vertical.GenerateMipmaps();
  EXPECT_NEAR(vertical.mipmaps.back().pixels[0].x, 1.0f / 3, 1e-6f);
}

TEST(TextureFiltering, TrilinearInterpolatesLevelsAndClampsLod) {
  Texture texture{
      2, 2, {{0, 0, 0}, {255, 255, 255}, {255, 255, 255}, {0, 0, 0}}};
  texture.GenerateMipmaps();
  EXPECT_FLOAT_EQ(
      texture.SampleLinear(0.25f, 0.75f, TextureFilter::kTrilinear, 0).x, 0);
  EXPECT_FLOAT_EQ(
      texture.SampleLinear(0.25f, 0.75f, TextureFilter::kTrilinear, 0.5f).x,
      0.25f);
  EXPECT_FLOAT_EQ(
      texture.SampleLinear(0.25f, 0.75f, TextureFilter::kTrilinear, 100).x,
      0.5f);
  EXPECT_FLOAT_EQ(
      texture.SampleLinear(0.25f, 0.75f, TextureFilter::kTrilinear, -1).x, 0);
  EXPECT_FLOAT_EQ(TextureLod({0.25f, 0}, {0, 0.125f}, 16, 16), 2);
}

TEST(TextureFiltering, RasterizerSelectsMipLevelFromUvDerivatives) {
  Texture texture{8, 8, {}};
  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) {
      texture.pixels.push_back((x + y) % 2 ? Color{255, 255, 255} : Color{});
    }
  }
  texture.GenerateMipmaps();
  auto triangle = PrepareTriangle({0, 0, 0, {1, 1, 1}, {0, 0}, 1},
                                  {4, 0, 0, {1, 1, 1}, {64, 0}, 1},
                                  {0, 4, 0, {1, 1, 1}, {0, 64}, 1});
  ASSERT_TRUE(triangle);
  FrameBuffer frame(4, 4);
  FragmentSettings settings;
  settings.mode = ShadingMode::kUnlit;
  settings.filter = TextureFilter::kTrilinear;
  frame.DrawPreparedTriangle(*triangle, {0, 3, 0, 3}, &texture, &settings);
  EXPECT_EQ(frame.Pixels()[0], 0xbcbcbcu);
  settings.filter = TextureFilter::kNearest;
  frame.Clear({});
  frame.DrawPreparedTriangle(*triangle, {0, 3, 0, 3}, &texture, &settings);
  EXPECT_NE(frame.Pixels()[0], 0xbcbcbcu);
}

TEST(NormalTransform, RemainsPerpendicularUnderNonuniformScaleAndRotation) {
  const auto model = RotationY(0.7f) * Scale({2, 3, 0.5f});
  const auto normal =
      TransformDirection(NormalMatrix(model), Normalize(Vec3{1, 1, 1}));
  EXPECT_NEAR(Dot(normal, TransformDirection(model, {1, -1, 0})), 0, 1e-6f);
  EXPECT_NEAR(Dot(normal, TransformDirection(model, {0, 1, -1})), 0, 1e-6f);
  EXPECT_THROW(NormalMatrix(Scale({1, 0, 1})), std::invalid_argument);
}

TEST(Lighting, HighlightDependsOnViewAndDoesNotLightBackFaces) {
  FragmentSettings settings;
  settings.light.direction = {0, 0, 1};
  settings.light.ambient = 0;
  settings.material.diffuse = {};
  settings.material.specular = {255, 255, 255};
  settings.camera_position = {0, 0, 3};
  const auto facing = ShadeBlinnPhong({}, {0, 0, 1}, {}, settings);
  EXPECT_FLOAT_EQ(facing.x, 1);
  settings.camera_position = {3, 0, 3};
  EXPECT_LT(ShadeBlinnPhong({}, {0, 0, 1}, {}, settings).x, facing.x);
  settings.light.direction = {0, 0, -1};
  EXPECT_FLOAT_EQ(ShadeBlinnPhong({}, {0, 0, 1}, {}, settings).x, 0);
}

TEST(Lighting, DiffuseAndAmbientRemainLinearUntilOutput) {
  FragmentSettings settings;
  settings.light.ambient = 0.5f;
  settings.material.specular = {};
  settings.light.direction = {0, 0, -1};
  const auto result = ShadeBlinnPhong({1, 1, 1}, {0, 0, 1}, {}, settings);
  EXPECT_FLOAT_EQ(result.x, 0.5f);
  EXPECT_EQ(ToSrgb(result).r, 188);
}

TEST(Lighting, MatteMaterialUsesColoredDiffuseLightWithoutViewDependence) {
  FragmentSettings settings;
  settings.material.diffuse = {200, 150, 100};
  settings.material.specular = {};
  settings.light.color = {100, 180, 255};
  settings.light.direction = {0, 0, 7};
  settings.light.ambient = 0.25f;
  settings.light.intensity = 0.8f;
  const Vec3 albedo{0.2f, 0.5f, 0.8f};
  const auto base = Multiply(albedo, ToLinear(settings.material.diffuse));
  const auto expected =
      base * 0.25f +
      Multiply(ToLinear(settings.light.color), base * 0.75f) * 0.8f;
  const auto lighting = PrepareLighting(settings);
  for (const auto view : {Vec3{0, 0, 5}, {2, 1, -3}, {0, 0, 0}}) {
    settings.camera_position = view;
    const auto actual =
        ShadeBlinnPhong(albedo, {0, 0, 3}, {}, settings, lighting);
    EXPECT_FLOAT_EQ(actual.x, expected.x);
    EXPECT_FLOAT_EQ(actual.y, expected.y);
    EXPECT_FLOAT_EQ(actual.z, expected.z);
  }
}

TEST(Lighting, FlatMatteFastPathMatchesGeneralTexturedPath) {
  const auto triangle = PrepareTriangle(
      {1, 1, 0, {0.2f, 0.4f, 0.6f}, {}, 0.5f, {1, 3, 2}, {-1, 1, 0.2f}},
      {36, 3, 0, {0.2f, 0.4f, 0.6f}, {}, 0.8f, {1, 3, 2}, {1, 1, -0.2f}},
      {4, 28, 0, {0.2f, 0.4f, 0.6f}, {}, 1, {1, 3, 2}, {-1, -1, 0.3f}});
  ASSERT_TRUE(triangle);
  Texture white{1, 1, {{255, 255, 255}}};
  ShadowMap shadow;
  shadow.size = 16;
  shadow.depth.resize(256);
  for (int i = 0; i < 256; ++i) {
    shadow.depth[i] = (i % 5 - 2) * 0.2f;
  }
  FragmentSettings settings;
  settings.mode = ShadingMode::kPhong;
  settings.material = {{210, 150, 180}, {}, 32};
  settings.light.color = {180, 200, 255};
  settings.light.ambient = 0.23f;
  settings.light.intensity = 0.7f;
  for (bool shadows : {false, true}) {
    settings.shadow_map = shadows ? &shadow : nullptr;
    for (const Vec3 light : {Vec3{1, 2, 3}, {-1, -2, -3}}) {
      settings.light.direction = light;
      FrameBuffer actual(37, 29), reference(37, 29);
      actual.DrawPreparedTriangle(*triangle, {0, 36, 0, 28}, nullptr,
                                  &settings);
      // A white texture preserves the albedo but forces general fragment
      // shading.
      reference.DrawPreparedTriangle(*triangle, {0, 36, 0, 28}, &white,
                                     &settings);
      EXPECT_TRUE(std::equal(actual.Pixels(), actual.Pixels() + 37 * 29,
                             reference.Pixels()));
      EXPECT_TRUE(std::equal(actual.Depth(), actual.Depth() + 37 * 29,
                             reference.Depth()));
    }
  }
}

TEST(ModernPipeline, MatchesAcrossLayoutsAndWorkersAfterClippingAndScaling) {
  auto fixture = MakeBenchmarkScene("clipped");
  fixture.model = RotationY(0.2f) * Scale({1.5f, 0.75f, 1});
  fixture.settings.shading = ShadingMode::kPhong;
  fixture.settings.ambient = 0.15f;
  FrameBuffer reference(65, 49), frame(65, 49);
  SoftwareRenderer single(1), parallel(4);
  single.Render(fixture.mesh, fixture.camera, fixture.settings, reference,
                nullptr, fixture.model);
  ASSERT_GT(std::count_if(reference.Pixels(), reference.Pixels() + 65 * 49,
                          [](uint32_t p) { return p != 0; }),
            0);
  for (int tile : {0, 16, 32}) {
    fixture.settings.tile_size = tile;
    parallel.Render(fixture.mesh, fixture.camera, fixture.settings, frame,
                    nullptr, fixture.model);
    EXPECT_TRUE(std::equal(reference.Pixels(), reference.Pixels() + 65 * 49,
                           frame.Pixels()));
  }
}

TEST(ModernPipeline, ClipsNormalsAndWorldPositionsWithoutQuantizing) {
  ClipVertex a{{-2, 0, 0, 1}, {}, {}, {0, 0, 0}, {0, 0, 0}};
  ClipVertex b{{0, -0.5f, 0, 1}, {}, {}, {2, 0, 0}, {2, 0, 0}};
  ClipVertex c{{0, 0.5f, 0, 1}, {}, {}, {0, 2, 0}, {0, 2, 0}};
  const auto polygon = ClipTriangle(a, b, c);
  ASSERT_EQ(polygon.size, 4u);
  for (size_t i = 0; i < polygon.size; ++i) {
    const auto& v = polygon.vertices[i];
    if (v.position.x == -1) {
      EXPECT_FLOAT_EQ(v.normal.x + v.normal.y, 1);
      EXPECT_FLOAT_EQ(v.world_position.x + v.world_position.y, 1);
    }
  }
}

TEST(ModernPipeline, FilteredTextureIsDeterministicAcrossTilesAndWorkers) {
  auto fixture = MakeBenchmarkScene("quad");
  fixture.settings.shading = ShadingMode::kUnlit;
  for (auto& v : fixture.mesh.vertices) {
    v.uv *= 12;
  }
  SoftwareRenderer one(1), many(4);
  FrameBuffer expected(65, 49), actual(65, 49);
  for (auto filter : {TextureFilter::kNearest, TextureFilter::kBilinear,
                      TextureFilter::kTrilinear}) {
    fixture.settings.filter = filter;
    fixture.settings.tile_size = 0;
    one.Render(fixture.mesh, fixture.camera, fixture.settings, expected,
               &fixture.texture);
    for (int tile : {0, 16, 32}) {
      fixture.settings.tile_size = tile;
      many.Render(fixture.mesh, fixture.camera, fixture.settings, actual,
                  &fixture.texture);
      EXPECT_TRUE(std::equal(expected.Pixels(), expected.Pixels() + 65 * 49,
                             actual.Pixels()));
    }
  }
}
