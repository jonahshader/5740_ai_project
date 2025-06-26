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
#include "preview.h"
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
  conf.frame_limit = 120;
  conf.friction = 10;
  conf.dt = 1 / 30.0f;
  auto game = std::make_shared<ContinuousSeek>(conf);
  conf.frame_limit = 120;
  auto preview_game = std::make_shared<ContinuousSeek>(conf);

  // auto trained_sol = train_openai(game);
  auto [update_callback, start_preview, stop_preview] = create_preview<obs::Simple>(preview_game);
  start_preview();
  // auto trained_sol = train_1_player_example(game, update_callback);
  auto trained_sol = train_openai(game, update_callback);

  std::cout << "Final trained model performance: " << trained_sol.fitness << std::endl;

  // make players

  std::cout << std::endl;
  std::vector<std::shared_ptr<model::Model<obs::Simple>>> players;
  // players.emplace_back(std::make_shared<model::Keyboard<obs::Simple>>());
  players.push_back(trained_sol.model);

  // conf.frame_limit *= 10;
  // game = std::make_shared<ContinuousSeek>(conf);
  // std::uint64_t seed = 123;
  // while (true) {
  //   game->init(seed++);
  //   // play game
  //   play_and_render(*game, players);
  // }

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  stop_preview();
  return 0;
}
