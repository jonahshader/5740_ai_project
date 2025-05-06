#pragma once

#include <random>

#include "jnb.h"
#include "pl_nn.h"
#include "model.h"

namespace model {

// TODO: make a version of this that is a SimpleModel.
// so just a StaticPLNet wrapper
class PLNNModel : public Model {
public:
  PLNNModel(std::mt19937 &rng);
  ~PLNNModel() = default;
  jnb::PlayerInput forward(const jnb::GameState &state, bool p1_perspective);
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void reset() override {} // not stateful
  void init(size_t input_size, size_t output_size, std::mt19937 &rng) override;
  std::shared_ptr<Model> clone() const override;
  std::string get_name() const override;

private:
  StaticPLNet<32, 2> net;
};

} // namespace model
