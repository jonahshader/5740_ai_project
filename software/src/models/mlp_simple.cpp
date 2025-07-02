#include "mlp_simple.h"

namespace model {

SimpleMLP::SimpleMLP(size_t hidden_size, size_t hidden_count)
    : hidden_size(hidden_size), hidden_count(hidden_count) {}

void SimpleMLP::mutate(std::mt19937 &rng, float mutation_rate) {
  net.mutate(rng, mutation_rate);
}

std::vector<ParamSpan> SimpleMLP::get_spans() {
  return net.get_spans();
}

void SimpleMLP::apply_spans() {
  net.decay(0.1f);
}

SimplePow2MLP::SimplePow2MLP(size_t hidden_size, size_t hidden_count)
    : hidden_size(hidden_size), hidden_count(hidden_count) {}

void SimplePow2MLP::mutate(std::mt19937 &rng, float mutation_rate) {
  net.mutate(rng, mutation_rate);
}

std::vector<ParamSpan> SimplePow2MLP::get_spans() {
  return net.get_spans();
}

void SimplePow2MLP::apply_spans() {
  net.clamp_parameters();
}

} // namespace model
