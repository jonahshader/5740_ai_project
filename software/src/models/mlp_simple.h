#pragma once

#include <random>

#include "jnb.h"
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
  std::string get_name() const override;

private:
  StaticNeuralNet<float, SIMPLE_INPUT_COUNT, 32, 2, SIMPLE_OUTPUT_COUNT> net{};
};

} // namespace jnb
