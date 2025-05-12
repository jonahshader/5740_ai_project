#include "training.h"

#include "games/jnb.h"
#include "models/mlp_simple.h"
#include "observation_types.h"
#include "optimizers/ga_funs.h"

#include <random>

using namespace ga;

void train(const std::string &map_filename) {
  jnb::JnBGame game(map_filename, 400);

  auto sample_obs = game.build_observation();

  ModelBuilder<obs::Simple> build_model =
      [&](std::mt19937 &rng) -> std::shared_ptr<model::Model<obs::Simple>> {
    auto new_model = std::make_shared<model::SimpleMLP>(32, 3);
    new_model->init(sample_obs[0], game.get_action_count(), rng);
    return new_model;
  };

  Config<obs::Simple> config;
  config.populate_fun = make_tournament<obs::Simple>(4);
  config.fitness_fun = make_game_fitness_2p<obs::Simple>(std::make_shared<jnb::JnBGame>(game));
  config.model_builder = build_model;
  config.prior_best_select = make_tournament_prior_best<obs::Simple>(2);
  config.fitness_logger = fitness_printer<obs::Simple>;

  config.prior_best_size = 0;
  config.mutation_rate = 0.001f;

  State<obs::Simple> state;
  init(state, config);
  run(state, config);
}
