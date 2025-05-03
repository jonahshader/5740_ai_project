#include "mlp_simple.h"

namespace model {

SimpleMLP::SimpleMLP(std::mt19937 &rng, int observation_size, int action_size) {
  net.init(rng, observation_size, 32, 2, action_size);
}

void SimpleMLP::forward(const std::vector<float> &observation, std::vector<float> &action) {
  net.forward(observation.data(), action.data());
}

void SimpleMLP::mutate(std::mt19937 &rng, float mutation_rate) {
  net.mutate(rng, mutation_rate);
}

void SimpleMLP::reset() {
  // nothing to reset. this model is not stateful
}

std::shared_ptr<Model> SimpleMLP::clone() const {
  return std::make_shared<SimpleMLP>(*this);
}

std::string SimpleMLP::get_name() const {
  return "SimpleMLP";
}

} // namespace model
