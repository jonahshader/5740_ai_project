#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "jnb.h"
#include "interfaces.h"
#include "pixel_game.h"

namespace jnb {

void run_game_with_models(const std::string &map_filename, uint64_t seed,
                          std::shared_ptr<Model> model1, std::shared_ptr<Model> model2);

void run_on_pl(const std::string &map_filename);

} // namespace jnb
