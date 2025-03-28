#include <string>
#include <cstring>
#include <memory>
#include <random>

#include "core_render.h"
#include "models/human.h"
#include "models/mlp_simple.h"
#include "optimizers/simple.h"

int main(int argc, char *argv[]) {
  std::string map_file = "jnb_map_simplest.tmx"; // default map file

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

  // {
  //   // human vs randomly init model
  //   std::mt19937 rng(0);

  //   auto p1 = std::make_shared<jnb::HumanModel>();
  //   // auto p2 = std::make_shared<jnb::HumanModel>();
  //   auto p2 = std::make_shared<jnb::SimpleMLPModel>(rng);

  //   // jnb::run_game(map_file.c_str(), 0);
  //   jnb::run_game_with_models(map_file, 0, p1, p2);
  // }

  {
    // train a model, then play against it
    jnb::GAConfig config{};
    config.seed = 1;
    config.select_fun = jnb::make_tournament(2);
    jnb::EvalConfig eval_config{};

    // lambda that spits out a randomly init model
    auto model_builder = [](std::mt19937 &rng) {
      return std::make_shared<jnb::SimpleMLPModel>(rng);
    };

    // load map
    jnb::TileMap map;
    map.load_from_file(map_file);

    jnb::GAState ga_state;
    jnb::init_state(ga_state, map, config, model_builder);

    // train
    jnb::ga_simple(ga_state, config, eval_config);

    // play against it
    auto p1 = std::make_shared<jnb::HumanModel>();
    // auto p1 = ga_state.current[0].model->clone();
    auto p2 = ga_state.current[0].model->clone(); // grab a copy of the best model

    jnb::run_game_with_models(map_file, 0, p1, p2);
  }

  return 0;
}
