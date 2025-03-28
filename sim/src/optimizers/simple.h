#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include "core.h"
#include "interfaces.h"

namespace jnb {

struct Solution {
  std::shared_ptr<Model> model{nullptr};
  int fitness{};
};

using Population = std::vector<Solution>;

// selection function takes refs to the current evaluated populaiton, next population, and rng.
// populates next based on current.
using Selection = std::function<void(const Population &, Population &, std::mt19937 &)>;

struct GAState {
  Population current{};
  Population next{};
  std::vector<std::shared_ptr<Model>> prior_best{};
  std::vector<std::shared_ptr<Model>> references{};
  std::vector<int> reference_fitness{};
  int gen{0};
  std::mt19937 rng{};
  TileMap map{};
};

struct EvalConfig {
  std::vector<uint64_t> seeds{0, 1, 5, 1337, 24};
  int frame_limit{400};
};

struct GAConfig {
  float mutation_rate{0.5f};
  bool taper_mutation_rate{true};
  int max_gen{128};
  int population_size{64};
  int model_history_size{5};
  int model_history_interval{5};
  uint64_t seed{0};
  int reference_count{5};
  int eval_interval{4};
};

using ModelBuilder = std::function<std::shared_ptr<Model>(std::mt19937 &)>;

// a function to initialize state using another builder function
void init_state(GAState &state, const TileMap &map, const GAConfig &config, const ModelBuilder &model_builder);

void ga_step_simple(GAState &state, const GAConfig &config, const EvalConfig &eval_config);

void ga_simple(GAState &state, const GAConfig &config, const EvalConfig &eval_config);

} // namespace jnb
