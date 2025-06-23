#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "games/seek.h"
#include "jnb_render.h"
#include "lodepng.h"
#include "models/human.h"
#include "models/mlp_map_lut.h"
#include "models/mlp_simple.h"
#include "models/pl_nn_model.h"
#include "nn_utils.h"
#include "observation_types.h"
#include "optimizers/simple.h"
#include "pixel_game.h"
#include "play.h"
#include "training.h"

int main(int argc, char *argv[]) {
  std::string map_file = "jnb_map3.tmx"; // default map file

  // parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--map") == 0) {
      // make sure we have a value after the --map flag
      if (i + 1 < argc) {
        map_file = argv[i + 1];
        i++; // skip the next argument since we've used it
      }
    }
  }

  // train_crossover_example(map_file);

  std::mt19937 rng(0);
  // auto fourier = make_gaussian_random_fourier_transform(rng, 1.0f, 4);
  // std::vector<float> fourier_tranform;
  // fourier(1.0f, fourier_tranform);
  // for (auto f : fourier_tranform) {
  //   std::cout << f << " ";
  // }
  // std::cout << std::endl;
  // fourier(1.5f, fourier_tranform);
  // for (auto f : fourier_tranform) {
  //   std::cout << f << " ";
  // }
  std::vector<FloatToVec> fourier_transforms;
  std::mt19937 temp_rng(324);
  fourier_transforms.push_back(make_gaussian_random_fourier_transform(temp_rng, 2.0f, 3));
  // auto game =
  //     std::make_shared<jnb::JnBGame>(map_file, jnb::Config(1, 4800, true, fourier_transforms));

  // auto game = std::make_shared<jnb::JnBGame>(map_file, jnb::Config(1, 4800, true, {}));
  ContinuousSeek::Config conf;
  // conf.absolute_inputs = false;
  conf.frame_limit = 60;
  conf.friction = 20;
  conf.dt = 1 / 30.0f;
  auto game = std::make_shared<ContinuousSeek>(conf);

  // auto trained_sol = train_openai(game);
  auto trained_sol = train_1_player_example(game);

  // {
  //   // temp manually making a good model lol
  //   Solution<obs::Simple> trained_sol;
  //   std::vector<obs::Simple> sample_obs;
  //   game->observe(sample_obs);
  //   trained_sol.model = std::make_shared<model::SimpleMLP>(2, 1);
  //   trained_sol.model->init(sample_obs[0], game->get_action_count(), temp_rng);
  //   // wipe out the params
  //   auto spans = trained_sol.model->get_spans();
  //   // just assume floats for now
  //   for (auto &span : spans) {
  //     if (std::holds_alternative<std::span<float>>(span)) {
  //       auto &f_span = std::get<std::span<float>>(span);
  //       std::fill(f_span.begin(), f_span.end(), 0.0f);
  //     } else {
  //       // throw error
  //       // throw std::runtime_error("Expected span of floats, got something else");
  //       std::cerr << "Expected span of floats, got something else" << std::endl;
  //     }
  //   }
  //   // first weight matrix
  //   auto &weights1 = std::get<std::span<float>>(spans[0]);
  //   // second weight matrix
  //   auto &weights2 = std::get<std::span<float>>(spans[2]);

  //   // wire up
  //   std::cout << weights1.size() << std::endl;
  //   std::cout << weights2.size() << std::endl;
  //   weights1[0 * sample_obs[0].size() + 2] = -1.0f;
  //   weights1[1 * sample_obs[0].size() + 2] = 1.0f;

  //   weights2[0 * 2 + 0] = 1.0f;
  //   weights2[1 * 2 + 1] = 1.0f;
  // }

  std::cout << "Final trained model performance: " << trained_sol.fitness << std::endl;

  // make players

  std::cout << std::endl;
  std::vector<std::shared_ptr<model::Model<obs::Simple>>> players;
  // players.emplace_back(std::make_shared<model::Keyboard<obs::Simple>>());
  players.push_back(trained_sol.model);

  conf.frame_limit *= 10;
  game = std::make_shared<ContinuousSeek>(conf);
  std::uint64_t seed = 123;
  while (true) {
    game->init(seed++);
    // play game
    play_and_render(*game, players);
  }

  // jnb::run_on_pl(map_file);

  // std::shared_ptr<jnb::Model> trained;
  // {
  //   // train a model, then play against it
  //   jnb::GAConfig config{};
  //   config.seed = 5;
  //   config.select_fun = jnb::make_tournament(4);
  //   config.reference_count = 2;
  //   config.model_history_size = 4;
  //   config.model_history_interval = 2;
  //   config.population_size = 128;
  //   config.mutation_rate = 0.03;
  //   config.max_gen = 128;
  //   jnb::EvalConfig eval_config{};
  //   eval_config.frame_limit = 900;
  //   eval_config.seed_count = 4;

  //   // lambda that spits out a randomly init model
  //   auto model_builder = [width = map.width,
  //                         height = map.height](std::mt19937 &rng) -> std::shared_ptr<jnb::Model>
  //                         {
  //     static int model_type = 0;

  //     // model_type = (model_type + 1) % 3;
  //     // switch (model_type) {
  //     //   case 0:
  //     //     return std::make_shared<jnb::SimpleMLP>(rng);
  //     //     break;
  //     //   case 1:
  //     //     return std::make_shared<jnb::MLPMapLutModel>(rng, width, height);
  //     //     break;
  //     //   case 2:
  //     //     return std::make_shared<jnb::PLNNModel>(rng);
  //     //     break;
  //     //   default:
  //     //     return std::make_shared<jnb::SimpleMLP>(rng);
  //     //     break;
  //     // }
  //     return std::make_shared<jnb::PLNNModel>(rng);
  //   };

  //   jnb::GAState ga_state;
  //   jnb::init_state(ga_state, map, config, model_builder);

  //   // train
  //   jnb::ga_simple(ga_state, config, eval_config);

  //   // play against it
  //   auto p1 = std::make_shared<jnb::HumanModel>();
  //   // auto p1 = ga_state.current[0].model->clone();
  //   trained = ga_state.current[0].model->clone(); // grab a copy of the best model

  //   jnb::run_game_with_models(map_file, 0, p1, trained);
  // }

  // {
  //   // have it play against itself
  //   auto p2 = trained->clone();

  //   jnb::run_game_with_models(map_file, 0, trained, p2);
  // }

  return 0;
}
