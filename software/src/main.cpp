#include <string>
#include <cstring>
#include <memory>
#include <vector>
#include <random>

#include "jnb_render.h"
#include "models/human.h"
#include "models/mlp_simple.h"
#include "models/mlp_map_lut.h"
#include "models/pl_nn_model.h"
#include "optimizers/simple.h"
#include "pixel_game.h"
#include "lodepng.h"
#include "play.h"
#include "observation_types.h"
#include "training.h"

int main(int argc, char *argv[]) {
  std::string map_file = "jnb_map_tb.tmx"; // default map file

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

  train(map_file);

  // // load map
  // jnb::TileMap map;
  // map.load_from_file(map_file);

  // {
  //   // human vs randomly init model
  //   std::mt19937 rng(0);

  //   auto p1 = std::make_shared<jnb::HumanModel>();
  //   // auto p2 = std::make_shared<jnb::HumanModel>();
  //   // auto p2 = std::make_shared<jnb::SimpleMLP>(rng);
  //   // auto p2 = std::make_shared<jnb::MLPMapLutModel>(rng, map.width, map.height);
  //   auto p2 = std::make_shared<jnb::PLNNModel>(rng);

  //   // jnb::run_game(map_file.c_str(), 0);
  //   jnb::run_game_with_models(map_file, 0, p1, p2);
  // }

  // // make players
  // std::mt19937 rng(0);
  // std::vector<std::shared_ptr<model::Model<obs::Simple>>> players;
  // players.emplace_back(std::make_shared<model::Keyboard<obs::Simple>>());
  // // players.emplace_back(std::make_shared<model::Keyboard<obs::Simple>>());
  // auto test_mlp = std::make_shared<model::SimpleMLP>(rng, 32, 2);
  // players.emplace_back(test_mlp);

  // // make game
  // jnb::JnBGame game(map_file, -1); // framelimit of -1 means run forever
  // std::vector<std::vector<float>> sample_observations = game.build_observation();
  // test_mlp->init(sample_observations[0], game.get_action_count(), rng);
  // game.init(123);
  // // play game
  // play_and_render(game, players);

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
