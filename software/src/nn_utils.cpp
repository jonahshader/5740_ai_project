#include "nn_utils.h"

#include <numbers>

std::function<void(float input, std::vector<float> &output)>
make_gaussian_random_fourier_transform(std::mt19937 &rng, float std_dev, size_t transform_count) {
  std::vector<float> transforms;
  transforms.reserve(transform_count);

  std::normal_distribution<float> dist(0.0f, std_dev);
  for (size_t i = 0; i < transform_count; ++i) {
    transforms.push_back(dist(rng));
  }

  return [transforms = std::move(transforms)](float input, std::vector<float> &output) {
    output.clear();
    output.reserve(transforms.size() * 2 + 1);
    // pass through input as-is
    output.push_back(input);
    // compute fourier transforms on input
    for (const auto &t : transforms) {
      output.push_back(std::cos(t * input));
      output.push_back(std::sin(t * input));
    }
  };
}

std::function<void(float input, std::vector<float> &output)>
make_pow_2_fourier_transform(float min_frequency, size_t transform_count) {
  std::vector<float> transforms;
  transforms.reserve(transform_count);

  for (size_t i = 0; i < transform_count; ++i) {
    float frequency = min_frequency * std::pow(2.0f, static_cast<float>(i));
    transforms.push_back(2.0f * std::numbers::pi_v<float> * frequency); // Convert to angular frequency
  }

  return [transforms = std::move(transforms)](float input, std::vector<float> &output) {
    output.clear();
    output.reserve(transforms.size() * 2 + 1);
    // pass through input as-is
    output.push_back(input);
    // compute fourier transforms on input
    for (const auto &t : transforms) {
      output.push_back(std::cos(t * input));
      output.push_back(std::sin(t * input));
    }
  };
}
