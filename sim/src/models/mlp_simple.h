#pragma once

#include <random>

#include "core.h"
#include "neural_net.h"
#include "interfaces.h"

namespace jnb {

class SimpleMLPModel : public Model {
public:
  SimpleMLPModel(std::mt19937 &rng);
  ~SimpleMLPModel() = default;
  PlayerInput forward(const GameState &state, bool p1_perspective) override;
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void reset() override;
  std::shared_ptr<Model> clone() const override;

private:
  NeuralNet<float, SIMPLE_INPUT_COUNT, 32, 2, SIMPLE_OUTPUT_COUNT> net{};
};

} // namespace jnb
