#pragma once

#include <random>

#include "core.h"

namespace jnb {

class Model {
public:
  virtual ~Model() = default;

  virtual PlayerInput forward(const GameState &state, bool p1_perspective) = 0;
  virtual void mutate(std::mt19937 &rng, float mutation_rate) = 0;
};

} // namespace jnb
