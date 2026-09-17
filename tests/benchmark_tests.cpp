#include <gtest/gtest.h>

#include <limits>

#include "core/sample_statistics.h"
#include "scene/benchmark_scenes.h"

namespace {
// Pixel comparison keeps channel errors separate; packed integer differences
// incorrectly treat carries between RGB channels as large image errors.
struct ImageError {
  int maximum = 0;
  double mean = 0;
  size_t changed_pixels = 0;
};
ImageError CompareImages(const uint32_t* actual, const uint32_t* expected,
                         size_t count) {
  ImageError error;
  for (size_t i = 0; i < count; ++i) {
    if (actual[i] != expected[i]) {
      ++error.changed_pixels;
    }
    for (int shift : {0, 8, 16}) {
      const int delta =
          std::abs(static_cast<int>((actual[i] >> shift) & 255) -
                   static_cast<int>((expected[i] >> shift) & 255));
      error.maximum = std::max(error.maximum, delta);
      error.mean += delta;
    }
  }
  if (count) {
    error.mean /= count * 3;
  }
  return error;
}
}  // namespace

TEST(BenchmarkStatistics, UsesMedianAndNearestRankP95) {
  const auto odd = SummarizeSamples({5, 1, 3});
  EXPECT_DOUBLE_EQ(odd.median, 3);
  EXPECT_DOUBLE_EQ(odd.p95, 5);
  const auto even = SummarizeSamples({4, 1, 2, 3});
  EXPECT_DOUBLE_EQ(even.median, 2.5);
  EXPECT_DOUBLE_EQ(even.p95, 4);
  std::vector<double> hundred;
  for (int i = 1; i <= 100; ++i) {
    hundred.push_back(i);
  }
  EXPECT_DOUBLE_EQ(SummarizeSamples(hundred).p95, 95);
  EXPECT_DOUBLE_EQ(SummarizeSamples({0}).median, 0);
  EXPECT_THROW(SummarizeSamples({}), std::invalid_argument);
  EXPECT_THROW(SummarizeSamples({-1}), std::invalid_argument);
  EXPECT_THROW(SummarizeSamples({std::numeric_limits<double>::quiet_NaN()}),
               std::invalid_argument);
}

TEST(ImageComparison, MeasuresChannelsIndependently) {
  const uint32_t a[] = {0x00ff00, 0x010203};
  const uint32_t b[] = {0x010000, 0x010204};
  const auto error = CompareImages(a, b, 2);
  EXPECT_EQ(error.maximum, 255);
  EXPECT_DOUBLE_EQ(error.mean, 257.0 / 6);
  EXPECT_EQ(error.changed_pixels, 2u);
}

TEST(BenchmarkImages, OverlapMatchesAnalyticReferenceImage) {
  constexpr int kSize = 64;
  auto fixture = MakeBenchmarkScene("overlap");
  FrameBuffer frame(kSize, kSize);
  SoftwareRenderer renderer(2);
  renderer.Render(fixture.mesh, fixture.camera, fixture.settings, frame);
  // Independent reference from projected extents at 90-degree FOV. No calls
  // to projection, clipping, or rasterization code construct this image.
  std::vector<uint32_t> expected(kSize * kSize);
  for (int y = 0; y < kSize; ++y) {
    for (int x = 0; x < kSize; ++x) {
      if (x >= 8 && x < 56 && y >= 8 && y < 56) {
        expected[y * kSize + x] = 0xff0000;
      }
      if (x >= 16 && x < 48 && y >= 16 && y < 48) {
        expected[y * kSize + x] = 0x00ff00;
      }
    }
  }
  const auto error =
      CompareImages(frame.Pixels(), expected.data(), expected.size());
  EXPECT_EQ(error.maximum, 0);
  EXPECT_EQ(error.changed_pixels, 0u);
  // Drawing order must not affect opaque surfaces at different depths.
  std::reverse(fixture.mesh.triangles.begin(), fixture.mesh.triangles.end());
  renderer.Render(fixture.mesh, fixture.camera, fixture.settings, frame);
  EXPECT_EQ(
      CompareImages(frame.Pixels(), expected.data(), expected.size()).maximum,
      0);
}

class BenchmarkFixture : public testing::TestWithParam<const char*> {};
TEST_P(BenchmarkFixture, ImageIsStableAcrossWorkersAndWarmupFrames) {
  auto fixture = MakeBenchmarkScene(GetParam());
  constexpr size_t kCount = 129 * 97;
  FrameBuffer reference(129, 97), actual(129, 97);
  const auto scene = fixture.Instantiate();
  SoftwareRenderer serial(1);
  const auto stats =
      serial.Render(scene, fixture.camera, fixture.settings, reference);
  EXPECT_GT(stats.input_triangles, 0u);
  EXPECT_GT(stats.raster_triangles, 0u);
  EXPECT_GT(std::count_if(reference.Pixels(), reference.Pixels() + kCount,
                          [](uint32_t p) { return p != 0; }),
            0);
  for (int workers : {2, 4}) {
    SoftwareRenderer parallel(workers);
    for (int tile_size : {0, 16, 32}) {
      fixture.settings.tile_size = tile_size;
      for (int frame = 0; frame < 3; ++frame) {
        parallel.Render(scene, fixture.camera, fixture.settings, actual);
        const auto error =
            CompareImages(actual.Pixels(), reference.Pixels(), kCount);
        EXPECT_EQ(error.maximum, 0)
            << "workers=" << workers << " frame=" << frame;
        EXPECT_DOUBLE_EQ(error.mean, 0);
      }
    }
  }
}
INSTANTIATE_TEST_SUITE_P(SceneSuite, BenchmarkFixture,
                         testing::Values("quad", "overlap", "clipped", "micro",
                                         "shrimp", "primitives"));

TEST(BenchmarkScenes, RejectsUnknownNames) {
  EXPECT_THROW(MakeBenchmarkScene("unknown"), std::invalid_argument);
}
