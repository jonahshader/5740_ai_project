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

// selection function takes refs to the current evaluated population, next population, and rng.
// populates next based on current.
using Selection = std::function<void(const Population &, Population &, std::mt19937 &)>;

const Selection select_best = [](const Population &current, Population &next, std::mt19937 &rng) {
  // find best
  auto best = current[0];
  for (int i = 1; i < current.size(); ++i) {
    auto &sol = current[i];
    if (sol.fitness > best.fitness) {
      best = sol;
    }
  }

  // clone best into next
  next.clear();
  for (int i = 0; i < current.size(); ++i) {
    next.emplace_back(Solution{best.model->clone(), 0});
  }
};

Selection make_tournament(int tournament_size);

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
  int seed_count{3};
  int frame_limit{400};
  bool recycle_seeds{false};
};

struct GAConfig {
  float mutation_rate{0.5f};
  bool taper_mutation_rate{true};
  int max_gen{128};
  bool run_until_stop{false};
  int tournament_size{2}; // TODO: integrate into system
  int population_size{64};
  int model_history_size{5};
  int model_history_interval{5};
  uint64_t seed{0};
  int reference_count{5};
  int eval_interval{4};
  Selection select_fun{select_best};
};

using ModelBuilder = std::function<std::shared_ptr<Model>(std::mt19937 &)>;

// a function to initialize state using another builder function
void init_state(GAState &state, const TileMap &map, const GAConfig &config,
                const ModelBuilder &model_builder);

void ga_step_simple(GAState &state, const GAConfig &config, const EvalConfig &eval_config);

void ga_simple(GAState &state, const GAConfig &config, const EvalConfig &eval_config);

} // namespace jnb
