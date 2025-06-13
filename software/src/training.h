#pragma once

#include <utility>

#include "optimizers/ga.h"
#include "observation_types.h"
#include "games/jnb.h"

namespace {
  using ga::Solution;
  using obs::Simple;
}

void train(const std::string &map_filename);

void train_crossover_example(const std::string &map_filename);

Solution<Simple> train_1_player_example(std::shared_ptr<Game<Simple>> game);

Solution<Simple> train_openai(std::shared_ptr<Game<Simple>> game);
