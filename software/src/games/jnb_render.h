#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "jnb.h"
#include "model.h"
#include "pixel_game.h"

namespace jnb {

void run_on_pl(const std::string &map_filename);

} // namespace jnb
