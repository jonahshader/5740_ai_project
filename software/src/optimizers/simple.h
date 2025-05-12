#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include "jnb.h"
#include "model.h"

namespace jnb {

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
  int tournament_size{2}; // TODO: this is used in hardware, but select_fun is used in software
  int population_size{64};
  int model_history_size{5};
  int model_history_interval{5};
  uint64_t seed{0};
  int reference_count{5};
  int eval_interval{4};
};

} // namespace jnb
