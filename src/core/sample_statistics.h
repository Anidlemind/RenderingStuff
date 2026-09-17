#ifndef RENDERER_SRC_CORE_SAMPLE_STATISTICS_H_
#define RENDERER_SRC_CORE_SAMPLE_STATISTICS_H_

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

struct SampleStatistics {
  double median;
  double p95;
};

// Median averages the middle pair. p95 uses the nearest-rank definition.
inline SampleStatistics SummarizeSamples(std::vector<double> samples) {
  if (samples.empty()) {
    throw std::invalid_argument("No timing samples");
  }
  for (double sample : samples) {
    if (!std::isfinite(sample) || sample < 0) {
      throw std::invalid_argument("Invalid timing sample");
    }
  }
  std::sort(samples.begin(), samples.end());
  const size_t middle = samples.size() / 2;
  const double median = samples.size() % 2
                            ? samples[middle]
                            : (samples[middle - 1] + samples[middle]) / 2;
  const size_t rank = static_cast<size_t>(std::ceil(0.95 * samples.size()));
  return {median, samples[rank - 1]};
}

#endif  // RENDERER_SRC_CORE_SAMPLE_STATISTICS_H_
