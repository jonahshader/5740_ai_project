#pragma once

#include <random>
#include <vector>
#include <bitset>
#include <cstdint>

namespace jnb {

template <int inputs, int outputs> struct BinaryStaticLayer {
  std::bitset<outputs * inputs> weights;
  std::int16_t bias[outputs];

  void init(std::mt19937 &rng) {
    std::uniform_int_distribution<int> dist(0, 1);
    std::uniform_int_distribution<int> bias_dist(-2, 2);
    for (int i = 0; i < outputs; ++i) {
      for (int j = 0; j < inputs; ++j) {
        weights[i * inputs + j] = dist(rng);
      }
      bias[i] = bias_dist(rng);
    }
  }
  
  void forward(const std::bitset &input, std::bitset &output) {
    for (int i = 0; i < outputs; ++i) {
      int current_output = bias[i];
      for (int j = 0; j < inputs; ++j) {
        current_output += weights[i * inputs + j] == input[j] ? 1 : -1;
      }
      output[i] = current_output > 0;
    }
  }

  void mutate(std::mt19937 &rng, float mutation_rate) {
    std::normal_distribution<float> chance(0.0f, 1.0f);

    // mutate weights
    for (int i = 0; i < weight.size(); ++i) {
      if (chance(rng) < mutation_rate) {
        weights[i].flip();
      }
    }
    // mutate bias
    // TODO: this should probably change.
    // mutation amount should be proportional to layer size
    for (int i = 0; i < outputs; ++i) {
      if (chance(rng) < mutation_rate) {
        bias[i] += chance(rng) > 0.5 ? 1 : -1;
      }
    }
  }
};


template <int inputs, int hidden_size, int hidden_count, int outputs>
struct BinaryMLPModel {
    BinaryStaticLayer<inputs, hidden_size> input_layer;
    BinaryStaticLayer<hidden_size, hidden_size> hidden_layers[hidden_count - 1];
    BinaryStaticLayer<hidden_size, outputs> output_layer;
    
    void init(std::mt19937 &rng) {
        // just init each layer individually
        input_layer.init(rng);
        for (int i = 0; i < hidden_count - 1; ++i) {
          hidden_layers[i].init(rng);
        }
        output_layer.init(rng);
    }
  
    void forward(const std::bitset<inputs> &input, std::bitset<outputs> &output) {
        // allocate buffers with maximum required size  
        std::bitset<hidden_size> buffer_a;
        std::bitset<hidden_size> buffer_b;
  
        // set up pointers for current and next buffers
        std::bitset<hidden_size> *current = &buffer_a;
        std::bitset<hidden_size> *next = &buffer_b;
  
        // forward through input layer
        input_layer.forward(input, *current);
  
        // forward through hidden layers
        for (int i = 0; i < hidden_count - 1; ++i) {
          hidden_layers[i].forward(*current, *next);
          // swap buffers
          std::swap(current, next);
        }
  
        // forward through output layer
        output_layer.forward(*current, output);
    }
};

}