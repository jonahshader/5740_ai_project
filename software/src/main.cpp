#include <string>
#include <cstring>
#include <memory>
#include <random>

#include "core_render.h"
#include "models/human.h"
#include "models/mlp_simple.h"
#include "models/mlp_map_lut.h"
#include "optimizers/simple.h"
#include "pixel_game.h"
#include "lodepng.h"

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

  // load map
  jnb::TileMap map;
  map.load_from_file(map_file);

  {
    // human vs randomly init model
    std::mt19937 rng(0);

    auto p1 = std::make_shared<jnb::HumanModel>();
    // auto p2 = std::make_shared<jnb::HumanModel>();
    // auto p2 = std::make_shared<jnb::SimpleMLPModel>(rng);
    auto p2 = std::make_shared<jnb::MLPMapLutModel>(rng, map.width, map.height);

    // jnb::run_game(map_file.c_str(), 0);
    jnb::run_game_with_models(map_file, 0, p1, p2);
  }

  std::shared_ptr<jnb::Model> trained;
  {
    // train a model, then play against it
    jnb::GAConfig config{};
    config.seed = 2;
    config.select_fun = jnb::make_tournament(2);
    config.reference_count = 2;
    config.model_history_size = 4;
    config.model_history_interval = 10;
    config.population_size = 64;
    jnb::EvalConfig eval_config{};
    eval_config.frame_limit = 360;
    config.mutation_rate = 0.2;
    config.max_gen = 64;

    // lambda that spits out a randomly init model
    auto model_builder = [width = map.width,
                          height = map.height](std::mt19937 &rng) -> std::shared_ptr<jnb::Model> {
      static int model_type = 0;

      model_type = (model_type + 1) % 2;
      switch (model_type) {
        case 0:
          return std::make_shared<jnb::SimpleMLPModel>(rng);
          break;
        case 1:
          return std::make_shared<jnb::MLPMapLutModel>(rng, width, height);
          break;
        default:
          return std::make_shared<jnb::SimpleMLPModel>(rng);
          break;
      }
    };

    jnb::GAState ga_state;
    jnb::init_state(ga_state, map, config, model_builder);

    // train
    jnb::ga_simple(ga_state, config, eval_config);

    // play against it
    auto p1 = std::make_shared<jnb::HumanModel>();
    // auto p1 = ga_state.current[0].model->clone();
    trained = ga_state.current[0].model->clone(); // grab a copy of the best model

    jnb::run_game_with_models(map_file, 0, p1, trained);
  }

  {
    // have it play against itself
    auto p2 = trained->clone();

    jnb::run_game_with_models(map_file, 0, trained, p2);
  }

  return 0;
}
