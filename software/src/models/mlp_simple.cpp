#include "mlp_simple.h"

namespace jnb {

SimpleMLPModel::SimpleMLPModel(std::mt19937 &rng) {
  net.init(rng);
}

PlayerInput SimpleMLPModel::forward(const GameState &state, bool p1_perspective) {
  // get an observation from the game state
  std::vector<float> observation_f;
  observe_state_simple(state, observation_f, p1_perspective);

  // net neural net output
  float output[SIMPLE_OUTPUT_COUNT];
  net.forward(observation_f.data(), output);

  // interpret output
  PlayerInput input{};

  if (output[0] < -1) {
    input.left = true;
  } else if (output[0] > 1) {
    input.right = true;
  }
  input.jump = output[1] > 0;

  return input;
}

void SimpleMLPModel::mutate(std::mt19937 &rng, float mutation_rate) {
  net.mutate(rng, mutation_rate);
}

void SimpleMLPModel::reset() {
  // nothing to reset. this model is not stateful
}

std::shared_ptr<Model> SimpleMLPModel::clone() const {
  return std::make_shared<SimpleMLPModel>(*this);
}

std::string SimpleMLPModel::get_name() const {
  return "SimpleMLPModel";
}

} // namespace jnb
