#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include "interfaces.h"

namespace jnb {

struct Solution {
  std::shared_ptr<Model> model{nullptr};
  std::optional<int> fitness{std::nullopt};
};

using Population = std::vector<Solution>;

struct GAState {
  Population current{};
  Population next{};
  int gen{0};
  std::mt19937 rng{};
};

struct GAConfig {
  float mutation_rate{0.1f};
  bool taper_mutation_rate{true};
  int max_gen{512};
  int population_size{256};
  uint64_t seed{0};
};

// a function to initialize state using another builder function
void init_state(GAState &state, const GAConfig &config, const std::function<std::shared_ptr<Model>(std::mt19937 &)> &model_builder);

void ga_step_simple(GAState &state, const GAConfig &config, Model &opponent);

} // namespace jnb
