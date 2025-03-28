#pragma once

#include <memory>
#include <random>

#include "core.h"

namespace jnb {

class Model {
public:
  virtual ~Model() = default;

  virtual PlayerInput forward(const GameState &state, bool p1_perspective) = 0;
  virtual void mutate(std::mt19937 &rng, float mutation_rate) = 0;
  // reset internal state. used when a model has recurrent connections or is otherwise stateful in
  // some way.
  virtual void reset() = 0;
  virtual std::shared_ptr<Model> clone() const = 0;

  // TODO: serialize, identify, deserialize
};

} // namespace jnb
