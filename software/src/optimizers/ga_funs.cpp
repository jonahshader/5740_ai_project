#include "ga_funs.h"

namespace ga {

Populate make_tournament(size_t size) {
  return [=](const Population &current, Population &next, std::mt19937 &rng) {
    std::uniform_int_distribution<int> dist(0, current.size() - 1);
    next.clear();
    for (size_t i = 0; i < current.size(); ++i) {
      // Track the index of the best solution instead of a reference
      int best_idx = dist(rng);
      for (size_t j = 1; j < size; ++j) {
        int other_idx = dist(rng);
        if (current[other_idx].fitness > current[best_idx].fitness) {
          best_idx = other_idx;
        }
      }

      // Copy the best solution to the next population
      next.emplace_back(current[best_idx]);
    }
  };
}

} // namespace ga
