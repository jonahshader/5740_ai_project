#include "mlp_tri.h"
namespace jnb {
MLPTriModel::MLPTriModel(std::mt19937 &rng) {
    net.init(rng);
}

PlayerInput MLPTriModel::forward(const GameState &state, bool p1_perspective) {
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
    // interpret output - adjusted slightly for trinary network
    PlayerInput input{};
    if (output[0] < -0.5) {
        input.left = true;
    } else if (output[0] > 0.5) {
        input.right = true;
    }
    input.jump = output[1] > 0;
    return input;
}

void MLPTriModel::mutate(std::mt19937 &rng, float mutation_rate) {
    net.mutate(rng, mutation_rate);
}

void MLPTriModel::reset() {
    // nothing to reset. this model is not stateful
}

std::shared_ptr<Model> MLPTriModel::clone() const {
    return std::make_shared<MLPTriModel>(*this);
}

std::string MLPTriModel::get_name() const {
    return "MLPTriModel";
}
} // namespace jnb