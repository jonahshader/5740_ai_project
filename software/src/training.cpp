#include "training.h"

#include "games/jnb.h"
#include "models/mlp_simple.h"
#include "observation_types.h"
#include "optimizers/crossover_funs.h"
#include "optimizers/crossover_types.h"
#include "optimizers/ga_funs.h"

#include <random>

using namespace ga;

void train(const std::string &map_filename) {
  jnb::JnBGame game(map_filename, 2, 400);

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

void train_crossover_example(const std::string &map_filename) {
  // define the game that the agents are to play in.
  // this hooks into the fitness function.
  jnb::JnBGame game(map_filename, 2, 400); // framelimit set to 400

  // the models need to know the shape of the game's observation,
  // so we build one and pass it to the model initialization.
  auto sample_obs = game.build_observation();

  // the ga config need a function that tells it how to create the
  // initial population. this is a function that takes in rng and returns
  // a shared_ptr to a model. in this build function, we construct the
  // SimpleMLP model, initialize it with the sample observation and game
  // action count, then return it.
  ModelBuilder<obs::Simple> build_model =
      [&](std::mt19937 &rng) -> std::shared_ptr<model::Model<obs::Simple>> {
    auto new_model = std::make_shared<model::SimpleMLP>(32, 3);
    new_model->init(sample_obs[0], game.get_action_count(), rng);
    return new_model;
  };

  // now we can populate the GA config
  Config<obs::Simple> config;
  config.model_builder = build_model;
  // i have prior best turned off in this example, but this can't be null so i set it
  config.prior_best_select = best_prior_best<obs::Simple>;
  config.prior_best_size = 0;
  config.mutation_rate = 0.001f;
  // here the game is getting wrapped and turned into a fitness function.
  // this particular fitness function is for 2 player games and incorporates
  // the prior best and reference models.
  config.fitness_fun = make_game_fitness_2p<obs::Simple>(std::make_shared<jnb::JnBGame>(game));
  // the fitness_logger is called once per generation. the fitness_printer computes
  // the min, max, and avg fitness per generation and prints it.
  // however, we could easily make one that writes stats to a file for later analysis.
  config.fitness_logger = fitness_printer<obs::Simple>;
  // the crossover function i'm using is a parameter-wise crossover, but the ga expects
  // solution-wise crossover. i can use the converter functions to turn the param-wise
  // crossover into a span-wise, then to vector-wise, then to sol-wise.
  // kinda gross, but this lets us define crossover functions of varying granularity,
  // then we can just convert them into the least granular type to pass to the ga config.
  auto sol_uniform_crossover =
      to_sol_crossover<obs::Simple>(to_vec_crossover(to_span_crossover(uniform_crossover)));
  // here im passing the solution-wise crossover function to a function that
  // make a population function using tournament select combined with the crossover.
  // it takes in the tournament size, the crossover, and the crossover proportion.
  config.populate_fun = make_tournament_with_crossover(4, sol_uniform_crossover, 0.8f);

  // above i convert an existing crossover function, but we could also define one inline.
  // here i made a crossover that takes 25% of p1 and 75% of p2, where p1 is a param
  // from model1, and p2 is the corresponding parameter from model2.
  auto crossover_25 = [](auto p1, auto p2, float sol1_relative_perf, std::mt19937 &rng) {
    // 25% p1, 75% p2
    // TODO: this might break on non-float models
    return (p1 / 4) + (3 * p2/4);
  };
  // i still need to convert this:
  auto sol_crossover_25 =
      to_sol_crossover<obs::Simple>(to_vec_crossover(to_span_crossover(crossover_25)));
  // and if i were to use this, i would hook it in here:
  // config.populate_fun = make_tournament_with_crossover(4, sol_crossover_25, 0.8f);

  // now i just need to make the ga state, init it, and run it
  State<obs::Simple> state;
  init(state, config);
  run(state, config);
  // should be training now
}
