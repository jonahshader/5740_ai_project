#pragma once

#include <utility>

#include "games/jnb.h"
#include "observation_types.h"
#include "optimizers/ga.h"
#include "preview.h"

namespace {
using ga::Solution;
using obs::Simple;
} // namespace

void train(const std::string &map_filename);

void train_crossover_example(const std::string &map_filename);

Solution<Simple> train_1_player_example(std::shared_ptr<Game<Simple>> game,
                                        ga::PreviewCallback<Simple> &&update_callback);

Solution<Simple> train_openai(std::shared_ptr<Game<Simple>> game,
                              ga::PreviewCallback<Simple> &&update_callback);
