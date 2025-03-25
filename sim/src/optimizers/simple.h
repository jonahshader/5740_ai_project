#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <queue>
#include <utility>
#include <vector>

#include "core.h"
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
  std::queue<std::shared_ptr<Model>> prior_best{};
  int gen{0};
  std::mt19937 rng{};
};

struct EvalConfig {
  std::vector<uint64_t> seeds{};
  int frame_limit{1000}; // about 16.6 seconds
};

struct GAConfig {
  float mutation_rate{0.1f};
  bool taper_mutation_rate{true};
  int max_gen{512};
  int population_size{256};
  int history_size{5};
  int history_interval{1};
  uint64_t seed{0};
};

// a function to initialize state using another builder function
GAState init_state(const GAConfig &config,
                   const std::function<std::shared_ptr<Model>(std::mt19937 &)> &model_builder);

void ga_step_simple(GAState &state, const GAConfig &config);

int evaluate(const TileMap &map, const EvalConfig &config, std::shared_ptr<Model> model,
               std::shared_ptr<Model> opponent);

} // namespace jnb
