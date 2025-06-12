#pragma once

#include <cstdint>
#include <memory>
#include <random>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace model {

// Covers all the possible parameter types that models use. Add more as needed.
using ParamSpan = std::variant<std::span<float>, std::span<int8_t>>;

// TODO: Might be able to have a CoreModel that has everything but the
// template stuff, then have Model extend CoreModel and add the template stuff.
// that way, stuff that doesn't need the templated functions can just take in
// a CoreModel instead of the complicated Model<ObsType> type. an example of
// a beneficiary is the crossover functions, which at the highest level
// take in two solutions and spit out a model, where all three are templated.
// these functions only care about getting the spans of the models, which
// doesn't require templates. 
template <typename ObsType>
class Model {
public:
  virtual ~Model() = default;

  virtual void mutate(std::mt19937 &rng, float mutation_rate) {}
  // reset internal state. used when a model has recurrent connections or is otherwise stateful in
  // some way.
  virtual void reset() {}
  virtual bool is_stateful() const {
    return false;
  }
  // sample_observation is purely just for the model to see the shape of a sample
  virtual void init(const ObsType &sample_observation, size_t output_size, std::mt19937 &rng) {}
  virtual void forward(const ObsType &observation, std::vector<float> &action) {}
  virtual std::shared_ptr<Model<ObsType>> clone() const = 0;
  virtual std::string get_name() const = 0;
  virtual std::vector<ParamSpan> get_spans() {
    return {};
  };
  virtual void apply_spans() {};
};

} // namespace model
