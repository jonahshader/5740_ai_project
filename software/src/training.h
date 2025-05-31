#pragma once

#include "optimizers/ga.h"
#include "observation_types.h"

namespace {
  using ga::Solution;
  using obs::Simple;
}

void train(const std::string &map_filename);

void train_crossover_example(const std::string &map_filename);

Solution<Simple> train_1_player_example(const std::string &map_filename);
