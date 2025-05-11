#pragma once

#include <cassert>
#include <random>

#include "jnb.h"
#include "pl_nn.h"
#include "model.h"
#include "observation_types.h"

namespace model {

// TODO: make a version of this that is a SimpleModel.
// so just a StaticPLNet wrapper
class PLNNModel : public Model<obs::Simple> {
public:
  PLNNModel() {}
  ~PLNNModel() = default;
  void forward(const obs::Simple &observation, std::vector<float> &action) {
    int observation_int[32];
    int output_int[32];

    // int index = 0;
    // auto &first = p1_perspective ? state.p1 : state.p2;
    // auto &second = p1_perspective ? state.p2 : state.p1;

    // init to zero
    std::fill_n(observation_int, 32, 0);
    std::fill_n(output_int, 32, 0);

    // observation[index++] = state.coin_pos.x * jnb::CELL_SIZE;
    // observation[index++] = state.coin_pos.y * jnb::CELL_SIZE;
    // // first
    // observation[index++] = first.x.to_float();
    // observation[index++] = first.y.to_float();
    // observation[index++] = first.x_vel.to_float();
    // observation[index++] = first.y_vel.to_float();
    // observation[index++] = first.dead_timeout == 0 ? 32 : -32;
    // // repeat for second
    // observation[index++] = second.x.to_float();
    // observation[index++] = second.y.to_float();
    // observation[index++] = second.x_vel.to_float();
    // observation[index++] = second.y_vel.to_float();
    // observation[index++] = second.dead_timeout == 0 ? 32 : -32;
    // // deltas
    // // observation[index++] = first.x.to_float() - second.x.to_float() > 0 ? 32 : -32;
    // // observation[index++] = first.y.to_float() - second.y.to_float() > 0 ? 32 : -32;
    // // observation[index++] = first.x.to_float() - state.coin_pos.x * CELL_SIZE > 0 ? 32 : -32;
    // // observation[index++] = first.y.to_float() - state.coin_pos.y * CELL_SIZE > 0 ? 32 : -32;
    // observation[index++] = (first.x.to_float() - second.x.to_float()) * 2;
    // observation[index++] = (first.y.to_float() - second.y.to_float()) * 2;
    // observation[index++] = (first.x.to_float() - state.coin_pos.x * jnb::CELL_SIZE) * 2;
    // observation[index++] = (first.y.to_float() - state.coin_pos.y * jnb::CELL_SIZE) * 2;

    assert(observation.size() <= 32);
    assert(action.size() == 3);

    // copy into observation_int
    for (size_t i = 0; i < observation.size(); ++i) {
      // TODO: match up the multiplier with what the pl nn does
      observation_int[i] = static_cast<int>(observation[i] * 512);
    }

    // run nn
    net.forward(observation_int, output_int, false);

    // // interpret output as a playerinput
    // jnb::PlayerInput pl_input;
    // pl_input.left = output[0] > 0;
    // pl_input.right = output[1] > 0;
    // pl_input.jump = output[2] > 0;

    // return pl_input;

    // copy into output
    for (size_t i = 0; i < action.size(); ++i) {
      // TODO: match up the multiplier with what the pl nn does
      action[i] = static_cast<float>(output_int[i]) / 32.0f;
    }
  }
  void mutate(std::mt19937 &rng, float mutation_rate) override {
    net.mutate(rng, mutation_rate);
  }
  void init(const obs::Simple &sample_observation, size_t output_size, std::mt19937 &rng) override {
    net.init(rng);
  }
  std::shared_ptr<Model<obs::Simple>> clone() const override {
    return std::make_shared<PLNNModel>(*this);
  }
  std::string get_name() const override {
    return "PLNNModel";
  }

private:
  StaticPLNet<32, 2> net;
};

} // namespace model
