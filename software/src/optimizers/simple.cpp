#include "simple.h"

#include <iostream>
#include <unordered_map>

namespace jnb {

Selection make_tournament(int tournament_size) {
  return [tournament_size](const Population &current, Population &next, std::mt19937 &rng) {
    // clear out next and populate from tournament victors
    next.clear();

    // dist for individual selection
    std::uniform_int_distribution<int> t_sel(0, current.size() - 1);
    for (int i = 0; i < current.size(); ++i) {
      // tourney
      auto best = current[t_sel(rng)];
      for (int j = 1; j < tournament_size; ++j) {
        auto &sol = current[t_sel(rng)];
        if (sol.fitness > best.fitness) {
          best = sol;
        }
      }

      // we have a victor. add it
      next.emplace_back(Solution{best.model->clone(), 0});
    }
  };
}

void init_state(GAState &state, const TileMap &map, const GAConfig &config,
                const ModelBuilder &model_builder) {
  // set map
  state.map = map;

  // init rng
  state.rng.seed(config.seed);

  // build initial population
  state.current.clear();
  for (int i = 0; i < config.population_size; ++i) {
    state.current.emplace_back(Solution{model_builder(state.rng), 0});
  }

  // prior_best starts off with random models
  for (int i = 0; i < config.model_history_size; ++i) {
    state.prior_best.emplace_back(model_builder(state.rng));
  }

  // // references are randomly initialized models for the purpose of global evaluation
  // for (int i = 0; i < config.reference_count; ++i) {
  //   state.references.emplace_back(model_builder(state.rng));
  // }
  for (int i = 0; i < state.prior_best.size(); ++i) {
    state.references.emplace_back(state.prior_best[i]);
  }

  // next should be clear
  state.next.clear();
}

int evaluate_single(GameState &state_with_map, uint64_t seed, const EvalConfig &config,
                    std::shared_ptr<Model> model, std::shared_ptr<Model> opponent,
                    bool flip_player_initial_position) {
  // reset models
  model->reset();
  opponent->reset();

  // reset game state
  reinit(state_with_map, seed);

  // flip p1 p2
  if (flip_player_initial_position) {
    std::swap(state_with_map.p1, state_with_map.p2);
  }

  // run for the specified number of frames
  for (int i = 0; i < config.frame_limit; ++i) {
    auto model_action = model->forward(state_with_map, true);
    auto opponent_action = opponent->forward(state_with_map, false);

    // apply actions to the game state
    update(state_with_map, model_action, opponent_action);
  }

  return get_fitness(state_with_map, true);
}

int evaluate(const TileMap &map, const EvalConfig &config, std::shared_ptr<Model> model,
             std::shared_ptr<Model> opponent, const std::vector<uint64_t> &seeds) {
  // if opponent is model, we know the outcome will be symmetric,
  // so just early return 0 fitness
  if (model == opponent) {
    std::cout << "Model pit against itself. Returning fitness=0" << std::endl;
    return 0;
  }

  // make a game state with the map
  GameState state;
  state.map = map;

  int total_fitness = 0;
  // run an eval per seed
  for (const auto &seed : seeds) {
    // run twice: once normally, and again with the initial player positions swapped.
    // this ensure symmetry for when the same model is used for both p1 and p2.
    total_fitness += evaluate_single(state, seed, config, model, opponent, false);
    total_fitness += evaluate_single(state, seed, config, model, opponent, true);
  }

  return total_fitness;
}

void eval_pop(GAState &state, const EvalConfig &eval_config) {
  // generate new seeds for this generation
  std::vector<uint64_t> seeds;
  std::uniform_int_distribution<uint64_t> seed_dist(0, UINT64_MAX);
  for (int i = 0; i < eval_config.seed_count; ++i) {
    seeds.push_back(seed_dist(state.rng));
  }
#pragma omp parallel for
  for (int sol_i = 0; sol_i < state.current.size(); ++sol_i) {
    auto &sol = state.current[sol_i];
    sol.fitness = 0;
    for (int opponent_i = 0; opponent_i < state.prior_best.size(); ++opponent_i) {
      auto &opponent = state.prior_best[opponent_i];

      sol.fitness += evaluate(state.map, eval_config, sol.model, opponent, seeds);
    }
  }

#pragma omp parallel for
  for (int sol_i = 0; sol_i < state.current.size(); ++sol_i) {
    auto &sol = state.current[sol_i];
    for (int opponent_i = 0; opponent_i < state.references.size(); ++opponent_i) {
      auto &opponent = state.references[opponent_i];

      sol.fitness += evaluate(state.map, eval_config, sol.model, opponent, seeds);
    }
  }
}

void ga_step_simple(GAState &state, const GAConfig &config, const EvalConfig &eval_config) {
  // evaluate the current population
  eval_pop(state, eval_config);

  // // for now, just grab the best
  // auto best = state.current[0];

  // for (int i = 1; i < state.current.size(); ++i) {
  //   auto &sol = state.current[i];
  //   if (sol.fitness > best.fitness) {
  //     best = sol;
  //   }
  // }

  // // clone best into next
  // // TODO: make this modular?
  // state.next.clear();
  // for (int i = 0; i < config.population_size; ++i) {
  //   state.next.emplace_back(Solution{best.model->clone(), 0});
  // }

  // run selection
  config.select_fun(state.current, state.next, state.rng);

  // mutate
  for (int i = 1; i < config.population_size; ++i) {
    float mutation_rate = config.mutation_rate;
    if (config.taper_mutation_rate)
      mutation_rate *= static_cast<float>(i) / (config.population_size - 1);
    state.next[i].model->mutate(state.rng, mutation_rate);
  }

  // TODO: had a cool idea. if taper_mutation_rate is enabled,
  // identify the mutation rate that was used (need to sture this per solution).
  // the next generation should be copied and mutated so that the median mutation
  // rate is the previous best solution's mutation rate. i.e., the highest
  // mutation rate will be 2*best_mutation_rate, and the lowest will be 0.
  // i think there is an evolutionary strategy that does something like this.
  // also, use a learning rate for this, so we don't jump immediately to the new
  // mutation rate.

  // add to prior best
  if (state.gen % config.model_history_interval == 0) {
    // identify best
    auto &best = state.current[0];
    for (int i = 1; i < state.current.size(); ++i) {
      auto &sol = state.current[i];
      if (sol.fitness > best.fitness) {
        best = sol;
      }
    }
    // push best, pop oldest
    state.prior_best.push_back(best.model);
    state.prior_best.erase(state.prior_best.begin());
  }

  // swap current and next
  std::swap(state.current, state.next);

  ++state.gen;
}

void ga_simple(GAState &state, const GAConfig &config, const EvalConfig &eval_config) {
  // ref seeds should be fixed, so we can just populate by counting
  std::vector<uint64_t> ref_seeds;
  for (int i = 0; i < eval_config.seed_count; ++i) {
    ref_seeds.push_back(i);
  }

  while (true) {
    ga_step_simple(state, config, eval_config);

    if (state.gen % config.eval_interval == 0) {
      // evaluate the best against every reference model
      auto &best = state.current[0];
      int best_fitness = 0;
      for (int i = 0; i < state.references.size(); ++i) {
        auto &reference = state.references[i];
        best_fitness += evaluate(state.map, eval_config, best.model, reference, ref_seeds);
      }

      // record the fitness
      state.reference_fitness.push_back(best_fitness);

      std::cout << "Generation " << state.gen << ": "
                << "Reference fitness: " << best_fitness << std::endl;

      // count the instance names
      std::unordered_map<std::string, int> instance_count;
      for (const auto &sol : state.current) {
        instance_count[sol.model->get_name()]++;
      }

      // print the instance counts sorted by name
      std::cout << "Instance counts:" << std::endl;
      std::vector<std::pair<std::string, int>> sorted_counts(instance_count.begin(),
                                                             instance_count.end());
      std::sort(sorted_counts.begin(), sorted_counts.end(),
                [](const auto &a, const auto &b) { return a.first < b.first; });
      for (const auto &pair : sorted_counts) {
        std::cout << pair.first << ": " << pair.second << std::endl;
      }
    }

    if (state.gen >= config.max_gen) {
      std::cout << "GA finished after " << state.gen << " generations." << std::endl;
      break;
    }
  }
}

} // namespace jnb
