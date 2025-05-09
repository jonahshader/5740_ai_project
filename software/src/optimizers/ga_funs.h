#pragma once

#include <memory>

#include "ga.h"
#include "game.h"

namespace ga {

Populate make_tournament(size_t size);

// these are PriorBestSelect functions. PriorBestSelect is defined in ga.h
Solution random_prior_best(const Population &evaled_pop, std::mt19937 &rng);
Solution best_prior_best(const Population &evaled_pop, std::mt19937 &rng);

PriorBestSelect make_tournament_prior_best(size_t size);

Fitness make_game_fitness_2p(std::shared_ptr<Game> game);


} // namespace ga
