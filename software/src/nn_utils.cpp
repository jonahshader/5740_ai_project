#include "nn_utils.h"

#include <numbers>

FloatToVec make_gaussian_random_fourier_transform(std::mt19937 &rng, float std_dev,
                                                  size_t transform_count) {
  std::vector<float> transforms;
  transforms.reserve(transform_count);

  std::normal_distribution<float> dist(0.0f, std_dev);
  for (size_t i = 0; i < transform_count; ++i) {
    transforms.push_back(dist(rng));
  }

  return [=](float input, std::vector<float> &output) {
    output.clear();
    output.reserve(transforms.size() * 2);
    // compute fourier transforms on input
    for (const auto &t : transforms) {
      output.push_back(std::cos(t * input));
      output.push_back(std::sin(t * input));
    }
  };
}

FloatToVec make_pow_2_fourier_transform(float min_frequency, size_t transform_count) {
  std::vector<float> transforms;
  transforms.reserve(transform_count);

  for (size_t i = 0; i < transform_count; ++i) {
    float frequency = min_frequency * std::pow(2.0f, static_cast<float>(i));
    transforms.push_back(2.0f * std::numbers::pi_v<float> *
                         frequency); // Convert to angular frequency
  }

  return [transforms = std::move(transforms)](float input, std::vector<float> &output) {
    output.clear();
    output.reserve(transforms.size() * 2);
    // compute fourier transforms on input
    for (const auto &t : transforms) {
      output.push_back(std::cos(t * input));
      output.push_back(std::sin(t * input));
    }
  };
}

float sample_triangle_dist(float radius, std::mt19937 &rng) {
  std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
  float u = uniform(rng);

  if (u < 0.5f) {
    // left side: from -radius to 0
    return -radius + radius * std::sqrt(2.0f * u);
  } else {
    // right side: from 0 to +radius
    return radius - radius * std::sqrt(2.0f * (1.0f - u));
  }
}

float sample_half_triangle_dist(float radius, std::mt19937 &rng) {
  std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
  float u = uniform(rng);

  // just the positive side
  return radius - radius * std::sqrt(u);
}
