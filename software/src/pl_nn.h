#pragma once

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>
#include <cstdint>
#include <iostream>

namespace jnb {

using p_t = std::int8_t;

p_t mutate_param(p_t param, std::mt19937 &rng, float mutation_rate, bool is_bias);
int compute_sum_abs_activation(int *inputs, int input_count);

template <int inputs, int outputs> struct StaticPLLayer {
  p_t weights[outputs][inputs];
  p_t bias[outputs];

  void mutate(std::mt19937 &rng, float mutation_rate) {
    // loop through each weight
    for (int i = 0; i < outputs; ++i) {
      for (int j = 0; j < inputs; ++j) {
        weights[i][j] = mutate_param(weights[i][j], rng, mutation_rate, false);
      }
    }

    // loop through each bias
    for (int i = 0; i < outputs; ++i) {
      bias[i] = mutate_param(bias[i], rng, mutation_rate, true);
    }
  }

  void init(std::mt19937 &rng) {
    // loop through each weight
    for (int i = 0; i < outputs; ++i) {
      for (int j = 0; j < inputs; ++j) {
        weights[i][j] = mutate_param(0, rng, 1.0f, false);
      }
    }
    // loop through each bias
    for (int i = 0; i < outputs; ++i) {
      bias[i] = mutate_param(0, rng, 1.0f, true);
    }
  }

  void forward(int *input, int *output, bool activate = true) {
    for (int i = 0; i < outputs; ++i) {
      output[i] = bias[i] * 32;
      for (int j = 0; j < inputs; ++j) {
        output[i] += weights[i][j] * input[j];
      }
      // activation function (ReLU)
      if (activate)
        output[i] = std::max(static_cast<int>(0), output[i]);
    }

    const int WEIGHTS_PER_NEURON_EXP = (int)round(std::log2((float)outputs));
    constexpr int NEURON_DATA_WIDTH = 12;
    const int SUM_TO_LOGIC_SHIFT = 2 + WEIGHTS_PER_NEURON_EXP - 5;

    // make mask based on NEURON_DATA_WIDTH
    int mask = (1 << NEURON_DATA_WIDTH) - 1;

    // arithmetic shift outputs to the right
    for (int i = 0; i < outputs; ++i) {
      output[i] = (output[i] >> SUM_TO_LOGIC_SHIFT);
      bool positive = output[i] >= 0;
      int positive_ver = positive ? output[i] : -output[i];
      // mask to get the smallest NEURON_DATA_WIDTH bits
      int masked = positive_ver & mask;
      // re-introduce sign
      output[i] = positive ? masked : -masked;
    }
  }
};

template <int hidden_size, int layer_count> struct StaticPLNet {
  StaticPLLayer<hidden_size, hidden_size> layers[layer_count];

  void init(std::mt19937 &rng) {
    // just init each layer individually
    for (int i = 0; i < layer_count; ++i) {
      layers[i].init(rng);
    }
  }

  void forward(int *input, int *output, bool debug_print = false) {
    // allocate buffers with maximum required size
    int buffer_a[hidden_size];
    int buffer_b[hidden_size];

    // set up pointers for current and next buffers
    int *current = buffer_a;
    int *next = buffer_b;

    if (debug_print) {
      auto sum_abs = compute_sum_abs_activation(input, hidden_size);
      std::cout << "input sum_abs: " << sum_abs << std::endl;
    }

    // forward through first layer
    layers[0].forward(input, current);

    if (debug_print) {
      auto sum_abs = compute_sum_abs_activation(current, hidden_size);
      std::cout << "layer 0 sum_abs: " << sum_abs << std::endl;
    }

    // forward through hidden layers
    for (int i = 1; i < layer_count - 1; ++i) {
      layers[i].forward(current, next);
      // swap buffers
      std::swap(current, next);

      if (debug_print) {
        auto sum_abs = compute_sum_abs_activation(current, hidden_size);
        std::cout << "layer " << i << " sum_abs: " << sum_abs << std::endl;
      }
    }

    // forward through output layer directly to the provided output, with no activation
    layers[layer_count - 1].forward(current, output, false);

    if (debug_print) {
      auto sum_abs = compute_sum_abs_activation(output, hidden_size);
      std::cout << "output layer sum_abs: " << sum_abs << std::endl;
    }
  }

  void mutate(std::mt19937 &rng, float mutation_rate) {
    // mutate all layers
    for (int i = 0; i < layer_count; ++i) {
      layers[i].mutate(rng, mutation_rate);
    }
  }
};

} // namespace jnb
