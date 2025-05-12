#include "mlp_simple.h"

namespace model {

SimpleMLP::SimpleMLP(size_t hidden_size, size_t hidden_count)
    : hidden_size(hidden_size), hidden_count(hidden_count) {}


void SimpleMLP::mutate(std::mt19937 &rng, float mutation_rate) {
  net.mutate(rng, mutation_rate);
}

} // namespace model
