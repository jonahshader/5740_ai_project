#pragma once

#include <random>

#include "jnb.h"
#include "neural_net.h"
#include "model.h"

namespace model {

// TODO: expose DynamicNeuralNet datatype as template T?
class SimpleMLP : public SimpleModel {
public:
  SimpleMLP(std::mt19937 &rng, size_t hidden_size, size_t hidden_count);
  ~SimpleMLP() = default;
  void forward(const std::vector<float> &observation, std::vector<float> &action) override;
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void reset() override {} // not stateful
  void init(size_t input_size, size_t output_size, std::mt19937 &rng) override;
  std::shared_ptr<Model> clone() const override;
  std::string get_name() const override {
    return "SimpleMLP";
  }

private:
  size_t hidden_size;
  size_t hidden_count;
  DynamicNeuralNet<float> net{};
};

} // namespace model
