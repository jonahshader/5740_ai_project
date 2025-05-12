#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "ga.h"
#include "game.h"
#include "play.h"

namespace ga {

// TODO: this can be private
template <typename ObsType>
Solution<ObsType> tournament_select_single(const Population<ObsType> &evaled_pop,
                                           size_t tournament_size, std::mt19937 &rng) {
  std::uniform_int_distribution<int> dist(0, evaled_pop.size() - 1);
  int best_idx = dist(rng);
  for (size_t j = 1; j < tournament_size; ++j) {
    int other_idx = dist(rng);
    if (evaled_pop[other_idx].fitness > evaled_pop[best_idx].fitness) {
      best_idx = other_idx;
    }
  }
  return evaled_pop[best_idx];
}

template <typename ObsType>
Populate<ObsType> make_tournament(size_t size) {
  return [=](const Population<ObsType> &current, Population<ObsType> &next, std::mt19937 &rng) {
    next.clear();
    for (size_t i = 0; i < current.size(); ++i) {
      // run a tournament and store the winner
      next.emplace_back(tournament_select_single(current, size, rng));
    }
  };
}

// these are PriorBestSelect functions. PriorBestSelect is defined in ga.h
template <typename ObsType>
Solution<ObsType> random_prior_best(const Population<ObsType> &evaled_pop, std::mt19937 &rng) {
  std::uniform_int_distribution<int> dist(0, evaled_pop.size() - 1);
  return evaled_pop[dist(rng)];
}

template <typename ObsType>
Solution<ObsType> best_prior_best(const Population<ObsType> &evaled_pop, std::mt19937 &rng) {
  auto best = evaled_pop[0];
  for (const auto &sol : evaled_pop) {
    if (sol.fitness > best.fitness) {
      best = sol;
    }
  }
  return best;
}

template <typename ObsType>
PriorBestSelect<ObsType> make_tournament_prior_best(size_t size) {
  return [=](const Population<ObsType> &evaled_pop, std::mt19937 &rng) {
    return tournament_select_single(evaled_pop, size, rng);
  };
}

/**
 * @brief Creates a fitness function for two player games.
 *
 * @param game the game
 * @return The constructed fitness function
 */
template <typename ObsType>
Fitness<ObsType> make_game_fitness_2p(std::shared_ptr<Game<ObsType>> game) {
  assert(game->get_player_count() == 2);
  return [=](Solution<ObsType> &sol, std::vector<std::shared_ptr<Model<ObsType>>> &refs,
             std::vector<std::shared_ptr<Model<ObsType>>> &prior_best,
             const std::vector<uint64_t> &seeds) {
    sol.prior_best_fitness = 0;
    sol.ref_fitness = 0;
    sol.fitness = 0;

    // play on a clone of the game to allow this lambda to run in parallel
    auto game_clone = game->clone();

    // copy or clone prior best models/ref models depending on if they are stateful
    std::vector<std::shared_ptr<model::Model<ObsType>>> prior_best_clone;
    for (auto &pb : prior_best) {
      if (pb->is_stateful()) {
        prior_best_clone.push_back(pb->clone());
      } else {
        prior_best_clone.push_back(pb);
      }
    }
    std::vector<std::shared_ptr<model::Model<ObsType>>> refs_clone;
    for (auto &ref : refs) {
      if (ref->is_stateful()) {
        refs_clone.push_back(ref->clone());
      } else {
        refs_clone.push_back(ref);
      }
    }

    for (auto &opponent : prior_best_clone) {
      for (auto seed : seeds) {
        game_clone->init(seed);
        std::vector<std::shared_ptr<model::Model<ObsType>>> models;
        models.push_back(sol.model);
        models.push_back(opponent);
        auto episode_fitness = play(*game_clone, models)[0];
        sol.fitness += episode_fitness;
        sol.prior_best_fitness += episode_fitness;
      }
    }

    for (auto &opponent : refs_clone) {
      for (auto seed : seeds) {
        game_clone->init(seed);
        std::vector<std::shared_ptr<model::Model<ObsType>>> models;
        models.push_back(sol.model);
        models.push_back(opponent);
        auto episode_fitness = play(*game_clone, models)[0];
        sol.fitness += episode_fitness;
        sol.ref_fitness += episode_fitness;
      }
    }
  };
}

template <typename ObsType>
void fitness_printer(size_t current_gen, const Population<ObsType> &pop) {
  std::cout << "Generation: " << current_gen << std::endl;
  // compute min, max, avg
  int min = pop[0].fitness;
  int max = pop[0].fitness;
  int sum = 0;
  for (const auto &sol : pop) {
    if (sol.fitness < min) {
      min = sol.fitness;
    }
    if (sol.fitness > max) {
      max = sol.fitness;
    }
    sum += sol.fitness;
  }
  int avg = sum / pop.size();
  std::cout << "Min: " << min << ", Max: " << max << ", Avg: " << avg << std::endl;
}

} // namespace ga
