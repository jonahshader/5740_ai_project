#pragma once

#include <random>
#include <vector>

namespace model {

// a staticly allocated neural network with a generic parameter type
template <typename T, int inputs, int outputs> struct StaticLayer {
  T weights[outputs][inputs];
  T bias[outputs];

  void init(std::mt19937 &rng) {
    // xavier/glorot initialization
    float stddev = std::sqrt(2.0f / (inputs + outputs));
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

  void mutate(std::mt19937 &rng, float mutation_rate) {
    // calculate the initial standard deviation used during initialization
    float init_stddev = std::sqrt(2.0f / (inputs + outputs));

    // scale mutation rate by the initial stddev
    float scaled_mutation_rate = mutation_rate * init_stddev;

    // create distribution for mutations
    std::normal_distribution<float> dist(0.0f, scaled_mutation_rate);

    // mutate weights
    for (int i = 0; i < outputs; ++i) {
      for (int j = 0; j < inputs; ++j) {
        weights[i][j] += dist(rng);
      }
      // mutate bias
      bias[i] += dist(rng);
    }
  }
};

template <typename T, int inputs, int hidden_size, int hidden_count, int outputs>
struct StaticNeuralNet {
  static_assert(hidden_count >= 1, "StaticNeuralNet must have at least one hidden layer");

  StaticLayer<T, inputs, hidden_size> input_layer;
  StaticLayer<T, hidden_size, hidden_size> hidden_layers[hidden_count - 1];
  StaticLayer<T, hidden_size, outputs> output_layer;

  void init(std::mt19937 &rng) {
    // just init each layer individually
    input_layer.init(rng);
    for (int i = 0; i < hidden_count - 1; ++i) {
      hidden_layers[i].init(rng);
    }
    output_layer.init(rng);
  }

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

  void mutate(std::mt19937 &rng, float mutation_rate) {
    // mutate all layers
    input_layer.mutate(rng, mutation_rate);
    for (int i = 0; i < hidden_count - 1; ++i) {
      hidden_layers[i].mutate(rng, mutation_rate);
    }
    output_layer.mutate(rng, mutation_rate);
  }
};

template <typename T>
struct DynamicLayer {
  int inputs;
  int outputs;
  std::vector<T> weights;
  std::vector<T> bias;

  T get_w(int input, int output) {
    return weights[output * inputs + input];
  }

  void set_w(int input, int output, T value) {
    weights[output * inputs + input] = value;
  }

  void init(std::mt19937 &rng, int inputs, int outputs) {
    // allocate memory
    this->inputs = inputs;
    this->outputs = outputs;
    weights.resize(inputs * outputs);
    bias.resize(outputs);

    // xavier/glorot initialization
    float stddev = std::sqrt(2.0f / (inputs + outputs));
    std::normal_distribution<float> dist(0.0f, stddev);
    for (int i = 0; i < outputs; ++i) {
      for (int j = 0; j < inputs; ++j) {
        set_w(j, i, dist(rng));
      }
      bias[i] = dist(rng);
    }
  }

  void forward(const T *input, T *output, bool activate = true) {
    for (int i = 0; i < outputs; ++i) {
      output[i] = bias[i];
      for (int j = 0; j < inputs; ++j) {
        output[i] += get_w(j, i) * input[j];
      }
      // activation function (ReLU)
      if (activate)
        output[i] = std::max(static_cast<T>(0), output[i]);
    }
  }

  void mutate(std::mt19937 &rng, float mutation_rate) {
    // calculate the initial standard deviation used during initialization
    float init_stddev = std::sqrt(2.0f / (inputs + outputs));

    // scale mutation rate by the initial stddev
    float scaled_mutation_rate = mutation_rate * init_stddev;

    // create distribution for mutations
    std::normal_distribution<float> dist(0.0f, scaled_mutation_rate);

    // mutate weights
    for (int i = 0; i < outputs; ++i) {
      for (int j = 0; j < inputs; ++j) {
        set_w(j, i, get_w(j, i) + dist(rng));
      }
      // mutate bias
      bias[i] += dist(rng);
    }
  }
};

template <typename T>
struct DynamicNeuralNet {
  std::vector<DynamicLayer<T>> layers;

  void init(std::mt19937 &rng, int inputs, int hidden_size, int hidden_count, int outputs) {
    layers.resize(hidden_count + 1);
    layers[0].init(rng, inputs, hidden_size);
    for (int i = 1; i < hidden_count; ++i) {
      layers[i].init(rng, hidden_size, hidden_size);
    }
    layers[hidden_count].init(rng, hidden_size, outputs);
  }

  void forward(const T *input, T *output) {
    // allocate buffers with maximum required size
    std::vector<T> buffer_a(layers[0].outputs);
    std::vector<T> buffer_b(layers[0].outputs);

    // set up pointers for current and next buffers
    T *current = buffer_a.data();
    T *next = buffer_b.data();

    // forward through input layer
    layers[0].forward(input, current);

    // forward through hidden layers
    for (int i = 1; i < layers.size(); ++i) {
      layers[i].forward(current, next);
      // swap buffers
      std::swap(current, next);
    }

    // forward through output layer directly to the provided output, with no activation
    layers.back().forward(current, output, false);
  }

  void mutate(std::mt19937 &rng, float mutation_rate) {
    // mutate all layers
    for (auto &layer : layers) {
      layer.mutate(rng, mutation_rate);
    }
  }

  std::string get_shape() {
    std::string shape = "DynamicNeuralNet: ";
    for (size_t i = 0; i < layers.size(); ++i) {
      shape += std::to_string(layers[i].inputs) + "x" + std::to_string(layers[i].outputs);
      if (i < layers.size() - 1) {
        shape += " -> ";
      }
    }
    return shape;
  }
};

} // namespace jnb
