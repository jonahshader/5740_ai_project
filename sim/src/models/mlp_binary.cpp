#include <cstdint>
#include <bitset>

#include "mlp_binary.h"

namespace jnb {

BinaryMLPModel::BinaryMLPModel(std::mt19937 &rng) {
  net.init(rng);
}

PlayerInput BinaryMLPModel::forward(const GameState &state, bool p1_perspective) {
  // get an observation from the game state
  std::vector<F4> observation;
  observe_state_simple(state, observation, p1_perspective);

  // net neural net output
  float output[3];
  std::bitset<3> output;
  net.forward(observation_f.data(), output);

  // interpret output
  PlayerInput input{};

  

//   if (output[0] < -1) {
//     input.left = true;
//   } else if (output[0] > 1) {
//     input.right = true;
//   }
//   input.jump = output[1] > 0;

  return input;
}

void BinaryMLPModel::mutate(std::mt19937 &rng, float mutation_rate) {
  net.mutate(rng, mutation_rate);
}

void BinaryMLPModel::reset() {
  // nothing to reset. this model is not stateful
}

std::shared_ptr<Model> BinaryMLPModel::clone() const {
  return std::make_shared<BinaryMLPModel>(*this);
}

std::string BinaryMLPModel::get_name() const {
  return "BinaryMLPModel";
}

} // namespace jnb
