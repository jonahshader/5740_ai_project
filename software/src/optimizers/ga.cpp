#include "ga.h"

namespace ga {

void init(State &state, const Config &config) {
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

void step(State &state, const Config &config) {
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

void ga(State &state, const Config &config) {
  do {
    step(state, config);
  } while (state.gen < config.max_gen);
}

} // namespace ga
