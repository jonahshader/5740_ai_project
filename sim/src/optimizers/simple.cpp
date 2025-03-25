#include "simple.h"

#include <iostream>

namespace jnb {

GAState init_state(const GAConfig &config,
                   const std::function<std::shared_ptr<Model>(std::mt19937 &)> &model_builder) {
  GAState state;

  // init rng
  state.rng.seed(config.seed);

  // build initial population
  state.current.clear();
  for (int i = 0; i < config.population_size; ++i) {
    state.current.emplace_back(Solution{model_builder(state.rng), std::nullopt});
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
               std::shared_ptr<Model> opponent) {
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
  for (auto seed : config.seeds) {
    // run twice: once normally, and again with the initial player positions swapped.
    // this ensure symmetry for when the same model is used for both p1 and p2.
    total_fitness += evaluate_single(state, seed, config, model, opponent, false);
    total_fitness += evaluate_single(state, seed, config, model, opponent, true);
  }
}

} // namespace jnb
