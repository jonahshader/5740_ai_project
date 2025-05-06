#include "pl_nn.h"

namespace model {
p_t mutate_param(p_t param, std::mt19937 &rng, float mutation_rate, bool is_bias) {
  std::uniform_real_distribution mutation_chance(0.0f, 1.0f);
  std::uniform_int_distribution<int> mutation_type(0, 7);
  if (mutation_chance(rng) < mutation_rate) {
    auto type = mutation_type(rng);
    switch (type) {
      case 0:
        param -= 1;
        break;
      case 1:
        param += 1;
        break;
      case 2:
        param -= 2;
        break;
      case 3:
        param += 2;
        break;
      case 4:
        param -= 3;
        break;
      case 5:
        param += 3;
        break;
      case 6:
        param -= 4;
        break;
      case 7:
        param += 4;
        break;
      default:
        break; // do nothing
    }

    // clamping is different for weight/bias
    if (is_bias) {
      // bias is clamped to [-7, 7]
      if (param < -7) {
        param = -7;
      } else if (param > 7) {
        param = 7;
      }
    } else {
      // weight is clamped to [-2, 2]
      if (param < -2) {
        param = -2;
      } else if (param > 2) {
        param = 2;
      }
    }
  }

  return param;
}

int compute_sum_abs_activation(int *inputs, int input_count) {
  // Check if input_count is valid
  if (input_count <= 0) {
    return 0; // Return 0 for invalid inputs
  }

  int sum = 0;

  // Calculate sum of absolute values
  for (int i = 0; i < input_count; i++) {
    sum += std::abs(inputs[i]);
  }

  // Return average (sum divided by count)
  return sum / input_count;
}

} // namespace model
