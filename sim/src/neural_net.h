#pragma once

#include <random>

namespace jnb {

// a staticly allocated neural network which is 
template <typename T, int inputs, int outputs> struct Layer {
  T weights[outputs][inputs];
  T bias[outputs];

  void init(std::mt19937 &rng) {
    // xavier/glorot initialization
    constexpr float stddev = std::sqrt(2.0f / (inputs + outputs));
    std::normal_distribution<float> dist(0.0f, stddev);
    for (int i = 0; i < outputs; ++i) {
      for (int j = 0; j < inputs; ++j) {
        weights[i][j] = dist(rng);
      }
      bias[i] = dist(rng);
    }
  }

  void forward(T *input, T *output, bool activate = true) {
    for (int i = 0; i < outputs; ++i) {
      output[i] = bias[i];
      for (int j = 0; j < inputs; ++j) {
        output[i] += weights[i][j] * input[j];
      }
      // activation function (ReLU)
      if (activate)
        output[i] = std::max(static_cast<T>(0), output[i]);
    }
  }
};

template <typename T, int inputs, int hidden_size, int hidden_count, int outputs> struct NeuralNet {
  static_assert(hidden_count >= 1, "NeuralNet must have at least one hidden layer");

  Layer<T, inputs, hidden_size> input_layer;
  Layer<T, hidden_size, hidden_size> hidden_layers[hidden_count - 1];
  Layer<T, hidden_size, outputs> output_layer;

  void forward(T *input, T *output) {
    // allocate buffers with maximum required size
    T buffer_a[hidden_size];
    T buffer_b[hidden_size];

    // set up pointers for current and next buffers
    T *current = buffer_a;
    T *next = buffer_b;

    // forward through input layer
    input_layer.forward(input, current);

    // forward through hidden layers
    for (int i = 0; i < hidden_count - 1; ++i) {
      hidden_layers[i].forward(current, next);
      // swap buffers
      std::swap(current, next);
    }

    // forward through output layer directly to the provided output, with no activation
    output_layer.forward(current, output, false);
  }

  constexpr int get_input_size() const {
    return inputs;
  }

  constexpr int get_output_size() const {
    return outputs;
  }
};

} // namespace jnb
