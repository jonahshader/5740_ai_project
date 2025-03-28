#include "selection.h"

namespace jnb {

Selection make_tournament(int tournament_size) {
  return [tournament_size](const Population &current, Population &next, std::mt19937 &rng) {
    // clear out next and populate from tournament victors
    next.clear();

    // dist for individual selection
    std::uniform_int_distribution<int> t_sel(0, current.size() - 1);
    for (int i = 0; i < current.size(); ++i) {
      // tourney
      auto best = current[t_sel(rng)];
      for (int j = 1; j < tournament_size; ++j) {
        auto &sol = current[t_sel(rng)];
        if (sol.fitness > best.fitness) {
          best = sol;
        }
      }

      // we have a victor. add it
      next.emplace_back(Solution{best.model->clone(), 0});
    }
  };
}

} // namespace jnb