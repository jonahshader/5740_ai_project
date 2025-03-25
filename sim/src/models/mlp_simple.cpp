#include "mlp_simple.h"

namespace jnb {

SimpleMLPModel::SimpleMLPModel(std::mt19937 &rng) {
  net.init(rng);
}

PlayerInput SimpleMLPModel::forward(const GameState &state, bool p1_perspective) {
  // get an observation from the game state
  std::vector<F4> observation;
  observe_state_simple(state, observation, p1_perspective);

  // convert observation to a float vector, for neural net compatibility
  std::vector<float> observation_f;
  observation_f.reserve(observation.size());
  for (auto o : observation) {
    observation_f.push_back(o.to_float());
  }
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

} // namespace jnb
