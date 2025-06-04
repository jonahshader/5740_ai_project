#pragma once

#include <cstdint>
#include <functional>
#include <numeric>

#include "ga.h"
#include "param_ops.h"

namespace es {

using model::Model;
using model::ParamSpan;
using model::ParamSpans;
using model::ParamVec;
using model::ParamVecs;

using std::size_t;
using std::uint64_t;

using param_ops::operator*=;
using param_ops::operator-=;

using Optimizer = std::function<void(ParamSpans &params, const ParamSpans &grad, float lr)>;

template <typename ObsType>
struct Config {
  // std of gaussian noise for parameter exploration
  float mutation_rate{0.01f};
  float learning_rate{0.01f};
  size_t max_gen{128};
  size_t population_size{64};
  size_t prior_best_size{4};
  size_t prior_best_interval{4};
  size_t references_size{4};
  uint64_t seed{0};
  ga::Fitness<ObsType> fitness_fun{nullptr};
  ga::ModelBuilder<ObsType> model_builder{nullptr};
  Optimizer optimizer{param_ops::axpy};
  size_t seeds_per_eval{4};
  ga::SeedChange seed_change{ga::NEVER};
  ga::Logger<ObsType> fitness_logger;
};

template <typename ObsType>
std::vector<float> fitness_shaping(const ga::Population<ObsType> &population) {
  const size_t lambda = population.size();

  // Extract fitness values and create index mapping
  std::vector<std::pair<int, size_t>> fitness_with_index;
  fitness_with_index.reserve(lambda);

  for (size_t i = 0; i < lambda; ++i) {
    fitness_with_index.emplace_back(population[i].fitness, i);
  }

  // Sort by fitness in descending order (best first)
  std::sort(fitness_with_index.begin(), fitness_with_index.end(),
            [](const auto &a, const auto &b) { return a.first > b.first; });

  // Create rank mapping: original_index -> rank (0-indexed)
  std::vector<size_t> ranks(lambda);
  for (size_t rank = 0; rank < lambda; ++rank) {
    size_t original_idx = fitness_with_index[rank].second;
    ranks[original_idx] = rank;
  }

  // Calculate weights using logarithmic rank transformation
  std::vector<float> weights(lambda);
  const float log_half_lambda_plus_one = std::log2(lambda / 2.0f + 1.0f);

  for (size_t i = 0; i < lambda; ++i) {
    const float log_rank_plus_one = std::log2(ranks[i] + 1.0f);
    weights[i] = std::max(0.0f, log_half_lambda_plus_one - log_rank_plus_one);
  }

  // Normalize weights and add uniform component
  const float weight_sum = std::accumulate(weights.begin(), weights.end(), 0.0f);
  const float uniform_component = 1.0f / lambda;

  if (weight_sum > 0.0f) {
    for (float &w : weights) {
      w = w / weight_sum + uniform_component;
    }
  } else {
    // Fallback: all uniform weights
    std::fill(weights.begin(), weights.end(), uniform_component);
  }

  return weights;
}

template <typename ObsType>
void init(ga::State<ObsType> &state, const Config<ObsType> &config) {
  // clear
  state = {};

  // init rng
  state.rng.seed(config.seed);

  // build initial population.
  // index 0 is the center
  state.current.emplace_back(ga::Solution{config.model_builder(state.rng)});
  // the rest are perturbations of the center.
  for (size_t i = 1; i < config.population_size; ++i) {
    auto clone = state.current[0].model->clone();
    clone->mutate(state.rng, config.mutation_rate);
    state.current.emplace_back(ga::Solution{clone});
    // state.current.emplace_back(ga::Solution{config.model_builder(state.rng)});
  }

  // prior best starts off with random models
  for (size_t i = 0; i < config.prior_best_size; ++i) {
    state.prior_best.emplace_back(config.model_builder(state.rng));
  }

  // references are randomly initialized models for the purpose of
  // anchoring agent learning against a non-moving ground truth,
  // and for global evaluation.
  for (size_t i = 0; i < config.references_size; ++i) {
    state.references.emplace_back(config.model_builder(state.rng));
  }

  // create initial eval seeds
  state.eval_seeds.reserve(config.seeds_per_eval);
  for (size_t i = 0; i < config.seeds_per_eval; ++i) {
    state.eval_seeds.push_back(config.seed + i);
  }
}

template <typename ObsType>
void step(ga::State<ObsType> &state, const Config<ObsType> &config) {
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
  auto &center = state.current[0].model;
  auto center_old = center->clone();
  auto center_spans = center->get_spans();
  ParamVecs grad = param_ops::zeros_like(center->get_spans());
  auto grad_spans = param_ops::to_spans(grad);

  std::vector<ParamSpans> spans;
  for (auto &sol : state.current) {
    spans.emplace_back(sol.model->get_spans());
  }

  auto weighted_fitness = fitness_shaping(state.current);
  auto weighted_fitness_spans = param_ops::to_spans(weighted_fitness);
  param_ops::weighted_average(spans, weighted_fitness, grad_spans);
  grad_spans -= center_spans;

  // apply es scaling
  float es_scale = 1.0f / config.mutation_rate;
  grad_spans *= es_scale;

  // take a step towards the positive direction
  config.optimizer(center_spans, grad_spans, config.learning_rate);

  // apply spans to ensure validity
  center->apply_spans();

  state.next.clear();
  state.next.reserve(state.current.size());
  // state.next[0].model = center;
  state.next.emplace_back(ga::Solution{center});
  for (size_t i = 1; i < state.current.size(); ++i) {
    // state.next[i].model = center->clone();
    // state.next[i].model->mutate(state.rng, config.mutation_rate);
    auto new_model = center->clone();
    new_model->mutate(state.rng, config.mutation_rate);
    state.next.emplace_back(ga::Solution{new_model});
  }

  // add to prior best
  if (config.prior_best_size > 0 && state.gen % config.prior_best_interval == 0) {
    state.prior_best.push_back(center_old);
    state.prior_best.erase(state.prior_best.begin());
  }

  // swap current and next
  std::swap(state.current, state.next);

  // increment generation
  ++state.gen;

  // if seed change is set to PER_GEN, then regenerate the seeds
  if (config.seed_change == ga::SeedChange::PER_GEN) {
    state.eval_seeds.clear();
    for (size_t i = 0; i < config.seeds_per_eval; ++i) {
      state.eval_seeds.push_back(config.seed + i);
    }
  }
}

template <typename ObsType>
void run(ga::State<ObsType> &state, const Config<ObsType> &config) {
  do {
    step(state, config);
  } while (state.gen < config.max_gen);
}

} // namespace es
