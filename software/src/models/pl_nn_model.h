#pragma once

#include <random>

#include "core.h"
#include "pl_nn.h"
#include "interfaces.h"

namespace jnb {

class PLNNModel : public Model {
public:
  PLNNModel(std::mt19937 &rng);
  ~PLNNModel() = default;
  PlayerInput forward(const GameState &state, bool p1_perspective) override;
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void reset() override;
  std::shared_ptr<Model> clone() const override;
  std::string get_name() const override;

private:
  StaticPLNet<32, 2> net;
};

} // namespace jnb
