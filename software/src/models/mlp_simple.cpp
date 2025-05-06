#include "mlp_simple.h"

namespace model {

SimpleMLP::SimpleMLP(std::mt19937 &rng, size_t hidden_size, size_t hidden_count)
    : hidden_size(hidden_size), hidden_count(hidden_count) {}

void SimpleMLP::forward(const std::vector<float> &observation, std::vector<float> &action) {
  net.forward(observation.data(), action.data());
}

void SimpleMLP::mutate(std::mt19937 &rng, float mutation_rate) {
  net.mutate(rng, mutation_rate);
}

void SimpleMLP::init(size_t input_size, size_t output_size, std::mt19937 &rng) {
  net.init(rng, input_size, hidden_size, hidden_count, output_size);
}

std::shared_ptr<Model> SimpleMLP::clone() const {
  return std::make_shared<SimpleMLP>(*this);
}

} // namespace model
