#pragma once

#include <random>

#include "../core.h"
#include "../binary_neural_net.h"
#include "../interfaces.h"

namespace jnb {

class BinaryMLPModel : public Model {
public:
BinaryMLPModel(std::mt19937 &rng);
  ~BinaryMLPModel() = default;
  PlayerInput forward(const GameState &state, bool p1_perspective) override;
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void reset() override;
  std::shared_ptr<Model> clone() const override;
  std::string get_name() const override;

private:
// x: 8, y: 8, other_x: 8, other_y: 8, 
BinaryMLPModel<SIMPLE_INPUT_COUNT , 16, 2, 3> net{};
};

} // namespace jnb
