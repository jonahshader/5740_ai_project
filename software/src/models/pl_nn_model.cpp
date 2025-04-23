#include "pl_nn_model.h"

namespace jnb {

PLNNModel::PLNNModel(std::mt19937 &rng) {
  net.init(rng);
}

PlayerInput PLNNModel::forward(const GameState &state, bool p1_perspective) {
  int observation[32];
  int output[32];

  int index = 0;
  auto &first = p1_perspective ? state.p1 : state.p2;
  auto &second = p1_perspective ? state.p2 : state.p1;

  // init to zero
  std::fill_n(observation, 32, 0);

  observation[index++] = state.coin_pos.x * CELL_SIZE;
  observation[index++] = state.coin_pos.y * CELL_SIZE;
  // first
  observation[index++] = first.x.to_float();
  observation[index++] = first.y.to_float();
  observation[index++] = first.x_vel.to_float();
  observation[index++] = first.y_vel.to_float();
  observation[index++] = first.dead_timeout == 0 ? 32 : -32;
  // repeat for second
  observation[index++] = second.x.to_float();
  observation[index++] = second.y.to_float();
  observation[index++] = second.x_vel.to_float();
  observation[index++] = second.y_vel.to_float();
  observation[index++] = second.dead_timeout == 0 ? 32 : -32;
  // deltas
  // observation[index++] = first.x.to_float() - second.x.to_float() > 0 ? 32 : -32;
  // observation[index++] = first.y.to_float() - second.y.to_float() > 0 ? 32 : -32;
  // observation[index++] = first.x.to_float() - state.coin_pos.x * CELL_SIZE > 0 ? 32 : -32;
  // observation[index++] = first.y.to_float() - state.coin_pos.y * CELL_SIZE > 0 ? 32 : -32;
  observation[index++] = (first.x.to_float() - second.x.to_float()) * 2;
  observation[index++] = (first.y.to_float() - second.y.to_float()) * 2;
  observation[index++] = (first.x.to_float() - state.coin_pos.x * CELL_SIZE) * 2;
  observation[index++] = (first.y.to_float() - state.coin_pos.y * CELL_SIZE) * 2;

  // run nn
  net.forward(observation, output, false);

  // interpret output as a playerinput
  PlayerInput pl_input;
  pl_input.left = output[0] > 0;
  pl_input.right = output[1] > 0;
  pl_input.jump = output[2] > 0;

  return pl_input;
}

void PLNNModel::mutate(std::mt19937 &rng, float mutation_rate) {
  net.mutate(rng, mutation_rate);
}

void PLNNModel::reset() {
  // nothing to reset. this model is not stateful
}

std::shared_ptr<Model> PLNNModel::clone() const {
  return std::make_shared<PLNNModel>(*this);
}

std::string PLNNModel::get_name() const {
  return "PLNNModel";
}

} // namespace jnb
