#pragma once

#include <memory>
#include <random>
#include <string>

namespace model {

class Model {
public:
  virtual ~Model() = default;

  virtual void mutate(std::mt19937 &rng, float mutation_rate) = 0;
  // reset internal state. used when a model has recurrent connections or is otherwise stateful in
  // some way.
  virtual void reset() = 0;
  virtual void init(size_t input_size, size_t output_size, std::mt19937 &rng) = 0;
  virtual std::shared_ptr<Model> clone() const = 0;
  virtual std::string get_name() const = 0;
};

// same as model, but has a basic forward pass
class SimpleModel : public Model {
public:
  virtual ~SimpleModel() = default;

  // forward pass
  // observation: input to the model
  // action: output of the model
  virtual void forward(const std::vector<float> &observation, std::vector<float> &action) = 0;
};

} // namespace model
