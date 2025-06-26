#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
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
using param_ops::operator+=;

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
  ga::Logger<ObsType> fitness_logger{nullptr};

  // Preview settings
  ga::PreviewCallback<ObsType> preview_callback{nullptr};
};

template <typename ObsType>
struct State {
  ga::Solution<ObsType> center{};
  ga::Population<ObsType> pop{};
  std::vector<std::shared_ptr<Model<ObsType>>> prior_best{};
  std::vector<std::shared_ptr<Model<ObsType>>> references{};
  int gen{0};
  std::mt19937 rng{};
  std::vector<uint64_t> eval_seeds{};
  float mutation_rate_scale{1.0f};
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
void init(State<ObsType> &state, const Config<ObsType> &config,
          std::shared_ptr<Model<ObsType>> center_model = nullptr) {
  // clear
  state = {};

  // init rng
  state.rng.seed(config.seed);

  // use existing center model or make one
  if (center_model) {
    state.center.model = center_model;
  } else {
    state.center.model = config.model_builder(state.rng);
    // // center starts at the center of init distribution, which is zero
    // auto spans = state.center.model->get_spans();
    // for (auto &span : spans) {
    //   if (std::holds_alternative<std::span<float>>(span)) {
    //     auto &f_span = std::get<std::span<float>>(span);
    //     // initialize to zero
    //     std::fill(f_span.begin(), f_span.end(), 0.0f);
    //   } else if (std::holds_alternative<std::span<std::int8_t>>(span)) {
    //     auto &i8_span = std::get<std::span<std::int8_t>>(span);
    //     // initialize to zero
    //     std::fill(i8_span.begin(), i8_span.end(), (std::int8_t)0);
    //   } else {
    //     // throw error
    //     throw std::runtime_error("Expected span of floats or int8_t, got something else");
    //   }
    // }
  }

  // population should be a multiple of 2
  if (config.population_size % 2 != 0) {
    throw std::runtime_error("Population size must be even.");
  }

  // build initial population.
  // the rest are perturbations of the center.
  float mr = config.mutation_rate * state.mutation_rate_scale;
  for (size_t i = 0; i < config.population_size / 2; ++i) {
    auto clone = state.center.model->clone();
    clone->mutate(state.rng, mr);
    state.pop.emplace_back(ga::Solution{clone});

    // build one with negative mutation
    auto neg_clone = state.center.model->clone();
    auto params = neg_clone->get_spans();      // center
    params -= clone->get_spans();              // center - (center + mutation) = -mutation
    params += state.center.model->get_spans(); // center - mutation
    neg_clone->apply_spans();
    state.pop.emplace_back(ga::Solution{neg_clone});
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
void step(State<ObsType> &state, const Config<ObsType> &config) {
  // evaluate the population.
  // this is the most expensive part of the algorithm, which happens to be
  // embarrassingly parallel, so we can use openmp to parallelize the loop.
#pragma omp parallel for
  for (int i = 0; i < state.pop.size(); ++i) {
    auto &sol = state.pop[i];
    sol.fitness = 0;
    sol.prior_best_fitness = 0;
    sol.ref_fitness = 0;
    config.fitness_fun(sol, state.references, state.prior_best, state.eval_seeds);
  }

  // log fitness
  if (config.fitness_logger) {
    config.fitness_logger(state.gen, state.pop);
  }

  // create the next population
  auto center_old = state.center.model->clone();
  auto center_spans = state.center.model->get_spans();
  ParamVecs grad = param_ops::zeros_like(center_spans);
  auto grad_spans = param_ops::to_spans(grad);

  std::vector<ParamSpans> spans;
  for (auto &sol : state.pop) {
    spans.emplace_back(sol.model->get_spans());
  }

  auto weighted_fitness = fitness_shaping(state.pop);
  auto weighted_fitness_spans = param_ops::to_spans(weighted_fitness);
  param_ops::weighted_average(spans, weighted_fitness, grad_spans);
  grad_spans -= center_spans;

  // apply es scaling
  float mr = config.mutation_rate * state.mutation_rate_scale;
  float es_scale = 1.0f / mr;
  grad_spans *= es_scale;

  // take a step towards the positive direction
  config.optimizer(center_spans, grad_spans, config.learning_rate);

  // apply spans to ensure validity
  state.center.model->apply_spans();

  // update preview with available models
  if (config.preview_callback) {
    auto preview_models = std::make_shared<std::vector<std::shared_ptr<Model<ObsType>>>>();

    // Add center model (current best)
    preview_models->push_back(state.center.model);

    // Add prior_best models
    for (auto &model : state.prior_best) {
      preview_models->push_back(model);
    }

    // Add reference models if needed
    for (auto &model : state.references) {
      preview_models->push_back(model);
    }

    config.preview_callback(preview_models);
  }

  // determine if we need to change mutation rate based on variance of fitness
  // TODO: try some other methods
  {
    bool flat_fitness = true;
    auto fit = state.pop[0].fitness;
    for (size_t i = 1; i < state.pop.size(); ++i) {
      if (state.pop[i].fitness != fit) {
        flat_fitness = false;
        break;
      }
    }

    if (flat_fitness) {
      // increase mutation rate
      state.mutation_rate_scale *= 1.1f;
    } else {
      // decrease mutation rate
      state.mutation_rate_scale *= 0.98f;
    }
    std::cout << "Mutation rate scale: " << state.mutation_rate_scale << std::endl;
    mr = config.mutation_rate * state.mutation_rate_scale;
  }

  state.pop.clear();
  state.pop.reserve(config.population_size);
  for (size_t i = 0; i < config.population_size / 2; ++i) {
    auto new_model = state.center.model->clone();
    new_model->mutate(state.rng, mr);
    state.pop.emplace_back(ga::Solution{new_model});
    // build one with negative mutation
    auto neg_model = state.center.model->clone();
    auto params = neg_model->get_spans();      // center
    params -= new_model->get_spans();          // center - (center + mutation) = -mutation
    params += state.center.model->get_spans(); // center - mutation
    neg_model->apply_spans();
    state.pop.emplace_back(ga::Solution{neg_model});
  }

  // add center to prior best
  if (config.prior_best_size > 0 && state.gen % config.prior_best_interval == 0) {
    state.prior_best.push_back(state.center.model);
    state.prior_best.erase(state.prior_best.begin());
  }

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
void run(es::State<ObsType> &state, const Config<ObsType> &config) {
  do {
    step(state, config);
  } while (state.gen < config.max_gen);
}

} // namespace es
