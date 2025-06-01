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
using ParamVec = std::variant<std::vector<float>, std::vector<int8_t>>;

using ParamSpans = std::vector<ParamSpan>;
using ParamVecs = std::vector<ParamVec>;

// this has been split into a non-templated base model, and a complete, templated model.
// this lets parts of the codebase avoid unnecessary templating if the templated function aren't
// needed.

class BaseModel {
public:
  virtual ~BaseModel() = default;

  virtual void mutate(std::mt19937 &rng, float mutation_rate) {}
  // reset internal state. used when a model has recurrent connections or is otherwise stateful in
  // some way.
  virtual void reset() {}
  virtual bool is_stateful() const {
    return false;
  }
  virtual std::string get_name() const = 0;
  virtual ParamSpans get_spans() {
    return {};
  };
  virtual void apply_spans() {};
  virtual std::shared_ptr<BaseModel> base_clone() const = 0;
};

template <typename ObsType>
class Model : public BaseModel {
public:
  // sample_observation is purely just for the model to see the shape of a sample
  virtual void init(const ObsType &sample_observation, size_t output_size, std::mt19937 &rng) {}
  virtual void forward(const ObsType &observation, std::vector<float> &action) {}
  virtual std::shared_ptr<Model<ObsType>> clone() const = 0;
};

} // namespace model
