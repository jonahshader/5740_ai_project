#pragma once

#include "simple.h"

namespace jnb {

const Selection select_best = [](const Population &current, Population &next, std::mt19937 &rng) {
  // find best
  auto best = current[0];
  for (int i = 1; i < current.size(); ++i) {
    auto &sol = current[i];
    if (sol.fitness > best.fitness) {
      best = sol;
    }
  }

  // clone best into next
  next.clear();
  for (int i = 0; i < current.size(); ++i) {
    next.emplace_back(Solution{best.model->clone(), 0});
  }
};


} // namespace jnb