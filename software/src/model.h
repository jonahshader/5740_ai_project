#pragma once

#include <memory>
#include <random>
#include <string>

#include "jnb.h"

class Model {
public:
  virtual ~Model() = default;

  virtual void forward(const std::vector<float> &observation, std::vector<float> &action) = 0;
  virtual void mutate(std::mt19937 &rng, float mutation_rate) = 0;
  // reset internal state. used when a model has recurrent connections or is otherwise stateful in
  // some way.
  virtual void reset() = 0;
  virtual std::shared_ptr<Model> clone() const = 0;
  virtual std::string get_name() const = 0;
};
