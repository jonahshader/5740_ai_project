#pragma once

#include <memory>
#include <vector>

#include "game.h"
#include "model.h"

std::vector<int> play(Game &game, const std::vector<std::shared_ptr<model::SimpleModel>> &models);

std::vector<int> play_and_render(Game &game, const std::vector<std::shared_ptr<model::SimpleModel>> &models);
