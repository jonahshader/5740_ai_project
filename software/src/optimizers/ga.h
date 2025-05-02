#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "model.h"

namespace ga {

// a struct to keep track of various fitness values associated with a model
struct Solution {
  std::shared_ptr<Model> model{nullptr};
  int fitness{0};
  int ref_fitness{0};
  int prior_best_fitness{0};
};

using Population = std::vector<Solution>;

// populate function takes refs to the current evaluated population, next population, and rng.
// populates next based on current.
using Populate =
    std::function<void(const Population &current, Population &next, std::mt19937 &rng)>;

// similarly, PriorBestSelect just grabs one solution
using PriorBestSelect = std::function<Solution(const Population &evaled_pop, std::mt19937 &rng)>;

using ModelBuilder = std::function<std::shared_ptr<Model>(std::mt19937 &)>;

using Fitness =
    std::function<int(Solution &sol, std::vector<std::shared_ptr<Model>> refs,
                      std::vector<std::shared_ptr<Model>> prior_best, std::vector<uint64_t> seeds)>;

using Logger = std::function<void(size_t current_gen, const Population &pop)>;

enum SeedChange { NEVER, PER_GEN };

struct State {
  Population current{};
  Population next{};
  std::vector<std::shared_ptr<Model>> prior_best{};
  std::vector<std::shared_ptr<Model>> references{};
  int gen{0};
  std::mt19937 rng{};
  std::vector<uint64_t> eval_seeds{};
};

struct Config {
  float mutation_rate{0.05f};
  bool taper_mutation_rate{true};
  size_t max_gen{128};
  size_t population_size{64};
  size_t prior_best_size{4};
  size_t prior_best_interval{4};
  size_t references_size{4};
  uint64_t seed{0};
  Populate populate_fun{nullptr};
  Fitness fitness_fun{};
  ModelBuilder model_builder{nullptr};
  size_t seeds_per_eval{4};
  SeedChange seed_change{NEVER};
  PriorBestSelect prior_best_select{nullptr};
  Logger fitness_logger{nullptr};
};

void init(State &state, const Config &config);

void step(State &state, const Config &config);

void ga(State &state, const Config &config);

} // namespace ga
