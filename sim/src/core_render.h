#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "core.h"
#include "interfaces.h"
#include "pixel_game.h"

namespace jnb {

void render(const GameState &state, SDL_Renderer *renderer,
            const std::vector<uint8_t> &spritesheet);

void run_game(const std::string &map_filename, uint64_t seed);

void run_game_with_models(const std::string &map_filename, uint64_t seed, std::shared_ptr<Model> model1, std::shared_ptr<Model> model2);

} // namespace jnb
