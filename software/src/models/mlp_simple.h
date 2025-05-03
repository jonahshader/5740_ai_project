#pragma once

#include <random>

#include "jnb.h"
#include "neural_net.h"
#include "model.h"

namespace model {

class SimpleMLP : public SimpleModel {
public:
  SimpleMLP(std::mt19937 &rng, int observation_size, int action_size);
  ~SimpleMLP() = default;
  void forward(const std::vector<float> &observation, std::vector<float> &action) override;
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void reset() override;
  std::shared_ptr<Model> clone() const override;
  std::string get_name() const override;

private:
  DynamicNeuralNet<float> net{};
};

} // namespace model
