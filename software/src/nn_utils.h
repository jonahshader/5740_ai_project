#pragma once

#include <cmath>
#include <functional>
#include <random>
#include <vector>

using FloatToVec = std::function<void(float input, std::vector<float> &output)>;

FloatToVec make_gaussian_random_fourier_transform(std::mt19937 &rng, float std_dev,
                                                  size_t transform_count);

FloatToVec make_pow_2_fourier_transform(float min_frequency, size_t transform_count);

float sample_triangle_dist(float radius, std::mt19937 &rng);

float sample_half_triangle_dist(float radius, std::mt19937 &rng);

// auto identity_act = [](auto x) { return x; };
// auto relu_act = [](auto x) {
//   using T = decltype(x);
//   return std::max(x, T{0});
// };
