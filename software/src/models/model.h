#pragma once

#include <memory>
#include <random>
#include <string>

namespace model {

template <typename ObsType> class Model {
public:
  virtual ~Model() = default;

  virtual void mutate(std::mt19937 &rng, float mutation_rate) {}
  // reset internal state. used when a model has recurrent connections or is otherwise stateful in
  // some way.
  virtual void reset() {}
  // sample_observation is purely just for the model to see the shape of a sample
  virtual void init(const ObsType &sample_observation, size_t output_size, std::mt19937 &rng) {}
  virtual void forward(const ObsType &observation, std::vector<float> &action) {}
  virtual std::shared_ptr<Model<ObsType>> clone() const = 0;
  virtual std::string get_name() const = 0;
};

} // namespace model
