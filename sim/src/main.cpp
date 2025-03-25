#include <string>
#include <cstring>
#include <memory>
#include <random>

#include "core_render.h"
#include "models/human.h"
#include "models/mlp_simple.h"

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

  std::mt19937 rng(0);

  auto p1 = std::make_shared<jnb::HumanModel>();
  // auto p2 = std::make_shared<jnb::HumanModel>();
  auto p2 = std::make_shared<jnb::SimpleMLPModel>(rng);

  // jnb::run_game(map_file.c_str(), 0);
  jnb::run_game_with_models(map_file, 0, p1, p2);
  return 0;
}
