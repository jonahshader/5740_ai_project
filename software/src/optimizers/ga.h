#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "model.h"

using model::Model;

namespace ga {

// a struct to keep track of various fitness values associated with a model
template <typename ObsType>
struct Solution {
  std::shared_ptr<Model<ObsType>> model{nullptr};
  int fitness{0};
  int ref_fitness{0};
  int prior_best_fitness{0};
};

template <typename ObsType>
using Population = std::vector<Solution<ObsType>>;

// populate function takes refs to the current evaluated population, next population, and rng.
// populates next based on current.
template <typename ObsType>
using Populate = std::function<void(const Population<ObsType> &current, Population<ObsType> &next,
                                    std::mt19937 &rng)>;

// similarly, PriorBestSelect just grabs one solution
template <typename ObsType>
using PriorBestSelect =
    std::function<Solution<ObsType>(const Population<ObsType> &evaled_pop, std::mt19937 &rng)>;

template <typename ObsType>
using ModelBuilder = std::function<std::shared_ptr<Model<ObsType>>(std::mt19937 &)>;

template <typename ObsType>
using Fitness =
    std::function<void(Solution<ObsType> &sol, std::vector<std::shared_ptr<Model<ObsType>>> &refs,
                       std::vector<std::shared_ptr<Model<ObsType>>> &prior_best,
                       const std::vector<uint64_t> &seeds)>;

template <typename ObsType>
using Logger = std::function<void(size_t current_gen, const Population<ObsType> &pop)>;

enum SeedChange { NEVER, PER_GEN };

template <typename ObsType>
struct State {
  Population<ObsType> current{};
  Population<ObsType> next{};
  std::vector<std::shared_ptr<Model<ObsType>>> prior_best{};
  std::vector<std::shared_ptr<Model<ObsType>>> references{};
  int gen{0};
  std::mt19937 rng{};
  std::vector<uint64_t> eval_seeds{};
};

template <typename ObsType>
struct Config {
  float mutation_rate{0.05f};
  bool taper_mutation_rate{true};
  size_t max_gen{128};
  size_t population_size{64};
  size_t prior_best_size{4};
  size_t prior_best_interval{4};
  size_t references_size{4};
  uint64_t seed{0};
  Populate<ObsType> populate_fun{nullptr};
  Fitness<ObsType> fitness_fun{nullptr};
  ModelBuilder<ObsType> model_builder{nullptr};
  size_t seeds_per_eval{4};
  SeedChange seed_change{NEVER};
  PriorBestSelect<ObsType> prior_best_select{nullptr};
  Logger<ObsType> fitness_logger{nullptr};
};

template <typename ObsType>
void init(State<ObsType> &state, const Config<ObsType> &config) {
  // clear
  state = {};

  // init rng
  state.rng.seed(config.seed);

  // build initial population
  for (int i = 0; i < config.population_size; ++i) {
    state.current.emplace_back(Solution{config.model_builder(state.rng), 0});
  }

  // prior best starts off with random models
  for (int i = 0; i < config.prior_best_size; ++i) {
    state.prior_best.emplace_back(config.model_builder(state.rng));
  }

  // references are randomly initialized models for the purpose of
  // anchoring agent learning against a non-moving ground truth,
  // and for global evaluation.
  for (int i = 0; i < config.references_size; ++i) {
    state.references.emplace_back(config.model_builder(state.rng));
  }

  // create initial eval seeds
  state.eval_seeds.reserve(config.seeds_per_eval);
  for (size_t i = 0; i < config.seeds_per_eval; ++i) {
    state.eval_seeds.push_back(config.seed + i);
  }
}

template <typename ObsType>
void step(State<ObsType> &state, const Config<ObsType> &config) {
  // evaluate the population.
  // this is the most expensive part of the algorithm, which happens to be
  // embarrassingly parallel, so we can use openmp to parallelize the loop.
#pragma omp parallel for
  for (int i = 0; i < state.current.size(); ++i) {
    auto &sol = state.current[i];
    sol.fitness = 0;
    sol.prior_best_fitness = 0;
    sol.ref_fitness = 0;
    config.fitness_fun(sol, state.references, state.prior_best, state.eval_seeds);
  }

  // log fitness
  if (config.fitness_logger) {
    config.fitness_logger(state.gen, state.current);
  }

  // create the next population
  state.next.clear();
  config.populate_fun(state.current, state.next, state.rng);

  // mutate
  // TODO: might want to use the original tapering logic over this
  std::uniform_real_distribution<float> mutation_ramp_dist(0.0f, 1.0f);
  for (int i = 1; i < state.next.size(); ++i) {
    float mutation_rate = config.mutation_rate;
    if (config.taper_mutation_rate) {
      mutation_rate *= mutation_ramp_dist(state.rng);
    }
    state.next[i].model->mutate(state.rng, mutation_rate);
  }

  // add to prior best
  if (state.gen % config.prior_best_interval == 0) {
    auto best = config.prior_best_select(state.next, state.rng);
    // push best, pop oldest
    state.prior_best.push_back(best.model);
    state.prior_best.erase(state.prior_best.begin());
  }

  // swap current and next
  std::swap(state.current, state.next);

  // increment generation
  ++state.gen;

  // if seed change is set to PER_GEN, then regenerate the seeds
  if (config.seed_change == SeedChange::PER_GEN) {
    state.eval_seeds.clear();
    for (size_t i = 0; i < config.seeds_per_eval; ++i) {
      state.eval_seeds.push_back(config.seed + i);
    }
  }
}

template <typename ObsType>
void ga(State<ObsType> &state, const Config<ObsType> &config) {
  do {
    step(state, config);
  } while (state.gen < config.max_gen);
}

} // namespace ga
