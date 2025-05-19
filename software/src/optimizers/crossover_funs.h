#pragma once

#include <random>

#include "crossover_types.h"

namespace ga {

// parameter level crossover functions

// auto uniform_crossover(auto param1, auto param2, float sol1_relative_perf, std::mt19937 &rng) {
//   std::bernoulli_distribution d(0.5f);
//   return d(rng) ? param1 : param2;
// }

// auto uniform_weighted_crossover(auto param1, auto param2, float sol1_relative_perf, std::mt19937
// &rng) {
//   std::bernoulli_distribution d(sol1_relative_perf);
//   return d(rng) ? param1 : param2;
// }

// not sure why, but it seems like these have to be defined as lambdas

auto uniform_crossover = [](auto p1, auto p2, float sol1_relative_perf, std::mt19937 &rng) {
  std::bernoulli_distribution d(0.5f);
  return d(rng) ? p1 : p2;
};

auto uniform_weighted_crossover = [](auto p1, auto p2, float sol1_relative_perf, std::mt19937 &rng) {
  std::bernoulli_distribution d(sol1_relative_perf);
  return d(rng) ? p1 : p2;
};

} // namespace ga
