#include "mlp_map_lut.h"

namespace jnb {

constexpr int EMBEDDING_SIZE = 4;
constexpr int INPUTS = (EMBEDDING_SIZE * 3) + SIMPLE_INPUT_COUNT;

MLPMapLutModel::MLPMapLutModel(std::mt19937 &rng, int map_width_tiles, int map_height_tiles) {
  net.init(rng, INPUTS, 32, 2, SIMPLE_OUTPUT_COUNT);
  player_embeddings.init(map_width_tiles, map_height_tiles, EMBEDDING_SIZE, rng);
  coin_embeddings.init(map_width_tiles, map_height_tiles, EMBEDDING_SIZE, rng);
}

PlayerInput MLPMapLutModel::forward(const GameState &state, bool p1_perspective) {
  // get an observation from the game state
  std::vector<float> observation_f;
  observe_state_simple(state, observation_f, p1_perspective);

  // read embeddings
  int p1_x = (state.p1.x.to_integer_floor() / CELL_SIZE) + CELL_SIZE / 2;
  int p1_y = (state.p1.y.to_integer_floor() / CELL_SIZE) + CELL_SIZE / 2;
  int p2_x = (state.p2.x.to_integer_floor() / CELL_SIZE) + CELL_SIZE / 2;
  int p2_y = (state.p2.y.to_integer_floor() / CELL_SIZE) + CELL_SIZE / 2;

  // order depends on perspective
  if (p1_perspective) {
    player_embeddings.get(observation_f, p1_x, p1_y);
    player_embeddings.get(observation_f, p2_x, p2_y);
  } else {
    player_embeddings.get(observation_f, p2_x, p2_y);
    player_embeddings.get(observation_f, p1_x, p1_y);
  }

  // coin position
  // TODO: can hard-code a smaller 1d table if we know the valid positions
  // beforehand, which we should, because we know the map size.
  coin_embeddings.get(observation_f, state.coin_pos.x, state.coin_pos.y);

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

void MLPMapLutModel::mutate(std::mt19937 &rng, float mutation_rate) {
  net.mutate(rng, mutation_rate);
  player_embeddings.mutate(rng, mutation_rate);
  coin_embeddings.mutate(rng, mutation_rate);
}

void MLPMapLutModel::reset() {
  // nothing to reset. this model is not stateful
}

std::shared_ptr<Model> MLPMapLutModel::clone() const {
  return std::make_shared<MLPMapLutModel>(*this);
}

std::string MLPMapLutModel::get_name() const {
  return "MLPMapLutModel";
}

} // namespace jnb
