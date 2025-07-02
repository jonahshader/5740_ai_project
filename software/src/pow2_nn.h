#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <random>
#include <span>
#include <vector>

#include "model.h"
#include "nn_utils.h"

namespace model {

// TODO: implement apply spans function
template <typename ActFun>
struct Pow2Layer {
  // config
  size_t inputs;
  size_t outputs;
  size_t weight_max_abs; // proportional to bits
  size_t bias_max_abs;
  int pre_act_shift_right_min;
  int pre_act_shift_right_max;
  ActFun act_fun;

  // trainable params
  std::vector<std::int8_t> encoded_weights; // power values
  std::vector<std::int16_t> bias;           // direct values
  std::int8_t pre_act_shift_right;

  explicit Pow2Layer(ActFun act_fun = ActFun{}) : act_fun(act_fun) {}

  int get_w(size_t input, size_t output) {
    return encoded_weights[output * inputs + input];
  }

  void set_w(size_t input, size_t output, int value) {
    encoded_weights[output * inputs + input] = value;
  }

  int effective_weight(std::int8_t encoded_weight) {
    if (encoded_weight == 0) {
      // zero is a special case as no shifting occurs
      return 0;
    }
    else if (encoded_weight > 0) {
      return 1 << (encoded_weight - 1);
    } else {
      return -(1 << (-encoded_weight - 1));
    }
  }

  int apply_weight(std::int8_t encoded_weight, int input) {
    if (encoded_weight == 0) {
      return 0;
    } else if (encoded_weight > 0) {
      return input << (encoded_weight - 1); // positive weight
    } else {
      return (-input) << (-encoded_weight - 1); // negative weight
    }
  }

  void init(std::mt19937 &rng, size_t inputs, size_t outputs, size_t weight_max_abs,
            size_t bias_max_abs, int pre_act_shift_right_min, int pre_act_shift_right_max) {
    // allocate memory
    this->inputs = inputs;
    this->outputs = outputs;
    this->weight_max_abs = weight_max_abs;
    this->bias_max_abs = bias_max_abs;
    this->pre_act_shift_right_min = pre_act_shift_right_min;
    this->pre_act_shift_right_max = pre_act_shift_right_max;
    encoded_weights.resize(inputs * outputs);
    bias.resize(outputs);

    // xavier/glorot init - target variance
    float target_variance = 2.0f / (inputs + outputs);

    std::bernoulli_distribution sign_dist(0.5f);

    // Sample weights using full range for maximum precision
    for (auto i = 0; i < outputs; ++i) {
      for (auto j = 0; j < inputs; ++j) {
        auto triangle_sample = sample_half_triangle_dist(std::pow(2, weight_max_abs), rng);
        int triangle_sample_pow = std::round(std::log2(triangle_sample));
        set_w(j, i, sign_dist(rng) ? triangle_sample_pow : -triangle_sample_pow);
      }
      bias[i] = 0; // no bias by default
    }

    // Calculate actual variance of sampled weights
    float actual_variance = 0.0f;
    for (auto i = 0; i < outputs; ++i) {
      for (auto j = 0; j < inputs; ++j) {
        auto encoded_weight = get_w(j, i);
        auto weight_value = effective_weight(encoded_weight);
        actual_variance += weight_value * weight_value;
      }
    }
    actual_variance /= (inputs * outputs);

    // Calculate shift needed to achieve target variance
    float scale_factor = std::sqrt(actual_variance / target_variance);
    pre_act_shift_right =
        std::max(pre_act_shift_right_min,
                 std::min(pre_act_shift_right_max, (int)std::round(std::log2(scale_factor))));
  }

  void forward(const std::span<int> input, std::span<int> output) {
    assert(input.size() >= inputs);
    assert(output.size() >= outputs);
    for (auto i = 0; i < outputs; ++i) {
      output[i] = bias[i];
      for (auto j = 0; j < inputs; ++j) {
        auto w = get_w(j, i);
        output[i] += apply_weight(w, input[j]);
      }
      // scale down output
      output[i] >>= pre_act_shift_right;
      // activate
      output[i] = act_fun(output[i]);
    }
  }

  void mutate(std::mt19937 &rng, float mutation_rate) {
    // calculate the initial standard deviation used during initialization
    float init_stddev = std::sqrt(2.0f / (inputs + outputs));

    // scale mutation rate by the initial stddev
    float scaled_mutation_rate = mutation_rate * init_stddev;

    std::bernoulli_distribution mutate_chance(scaled_mutation_rate);
    std::bernoulli_distribution sign_dist(0.5f); // interpreted as either +1 or -1
    for (auto i = 0; i < outputs; ++i) {
      for (auto j = 0; j < inputs; ++j) {
        if (mutate_chance(rng))
          set_w(j, i, get_w(j, i) + sign_dist(rng) ? 1 : -1);
      }
      // mutate bias
      // not sure if this should have the same mutation probability
      if (mutate_chance(rng))
        bias[i] += sign_dist(rng) ? 1 : -1;
    }

    // mutate pre_act_shift_right
    if (mutate_chance(rng)) {
      if (sign_dist(rng)) {
        pre_act_shift_right = std::min(pre_act_shift_right + 1, pre_act_shift_right_max);
      } else {
        pre_act_shift_right = std::max(pre_act_shift_right - 1, pre_act_shift_right_min);
      }
    }
  }

  void decay_random(std::mt19937 &rng, float probability) {
    std::bernoulli_distribution decay_dist(probability);
    for (auto i = 0; i < outputs; ++i) {
      for (auto j = 0; j < inputs; ++j) {
        if (decay_dist(rng)) {
          // decay towards zero
          auto value = get_w(j, i);
          if (value > 0) {
            set_w(j, i, value - 1);
          } else if (value < 0) {
            set_w(j, i, value + 1);
          }
        }
      }
      if (decay_dist(rng)) {
        auto value = bias[i];
        if (value > 0) {
          bias[i] = value - 1;
        } else if (value < 0) {
          bias[i] = value + 1;
        }
      }
    }
  }

  void clamp_parameters() {
    // Clamp weights to valid range
    for (auto i = 0; i < outputs; ++i) {
      for (auto j = 0; j < inputs; ++j) {
        auto value = get_w(j, i);
        if (value > static_cast<int>(weight_max_abs)) {
          set_w(j, i, weight_max_abs);
        } else if (value < -static_cast<int>(weight_max_abs)) {
          set_w(j, i, -weight_max_abs);
        }
      }
      // Clamp bias to valid range
      if (bias[i] > static_cast<int>(bias_max_abs)) {
        bias[i] = bias_max_abs;
      } else if (bias[i] < -static_cast<int>(bias_max_abs)) {
        bias[i] = -bias_max_abs;
      }
    }
  }

  std::vector<ParamSpan> get_spans() {
    std::vector<ParamSpan> spans;
    spans.push_back(std::span<std::int8_t>(encoded_weights.data(), encoded_weights.size()));
    spans.push_back(std::span<std::int16_t>(bias.data(), bias.size()));
    spans.push_back(std::span<std::int8_t>(&pre_act_shift_right, 1));
    return spans;
  }
};

template <typename HiddenActFun, typename OutputActFun>
struct Pow2NeuralNet {
  std::vector<Pow2Layer<HiddenActFun>> hidden_layers;
  Pow2Layer<OutputActFun> output_layer;

  // Default constructor
  Pow2NeuralNet() : output_layer(OutputActFun{}) {}

  void init(std::mt19937 &rng, size_t inputs, size_t hidden_size, size_t hidden_count,
            size_t outputs, size_t weight_max_abs = 4, size_t bias_max_abs = 1000,
            int pre_act_shift_right_min = 0, int pre_act_shift_right_max = 8,
            HiddenActFun hidden_act_fun = HiddenActFun{},
            OutputActFun output_act_fun = OutputActFun{}) {

    hidden_layers.resize(hidden_count);

    // Initialize first hidden layer (takes network input)
    hidden_layers[0] = Pow2Layer<HiddenActFun>(hidden_act_fun);
    hidden_layers[0].init(rng, inputs, hidden_size, weight_max_abs, bias_max_abs,
                          pre_act_shift_right_min, pre_act_shift_right_max);

    // Initialize remaining hidden layers
    for (size_t i = 1; i < hidden_count; ++i) {
      hidden_layers[i] = Pow2Layer<HiddenActFun>(hidden_act_fun);
      hidden_layers[i].init(rng, hidden_size, hidden_size, weight_max_abs, bias_max_abs,
                            pre_act_shift_right_min, pre_act_shift_right_max);
    }

    // Initialize output layer
    output_layer = Pow2Layer<OutputActFun>(output_act_fun);
    output_layer.init(rng, hidden_size, outputs, weight_max_abs, bias_max_abs,
                      pre_act_shift_right_min, pre_act_shift_right_max);

    std::cout << "Initialized Pow2 nn with inputs: " << inputs << " outputs: " << outputs
              << " hidden_count: " << hidden_count << " hidden_size: " << hidden_size << std::endl;
  }

  void forward(const std::span<int> input, std::span<int> output) {
    if (hidden_layers.empty()) {
      // Special case: no hidden layers, just output layer
      output_layer.forward(input, output);
      return;
    }

    // Calculate maximum buffer size needed
    size_t max_size = hidden_layers[0].inputs;
    for (const auto &layer : hidden_layers) {
      max_size = std::max(max_size, layer.outputs);
    }
    max_size = std::max(max_size, output_layer.outputs);

    // Allocate buffers
    std::vector<int> buffer_a(max_size);
    std::vector<int> buffer_b(max_size);

    // Set up spans for current and next buffers
    std::span<int> current(buffer_a.data(), hidden_layers[0].outputs);
    std::span<int> next(buffer_b.data(), max_size);

    // Forward through first hidden layer
    hidden_layers[0].forward(input, current);

    // Forward through remaining hidden layers
    for (size_t i = 1; i < hidden_layers.size(); ++i) {
      // Resize the next span to match the current layer's output size
      next = std::span<int>(next.data(), hidden_layers[i].outputs);
      hidden_layers[i].forward(current, next);

      // Swap buffers
      std::swap(current, next);
      // Update current span size for next iteration
      current = std::span<int>(current.data(), hidden_layers[i].outputs);
    }

    // Forward through output layer
    output_layer.forward(current, output);
  }

  void mutate(std::mt19937 &rng, float mutation_rate) {
    for (auto &layer : hidden_layers) {
      layer.mutate(rng, mutation_rate);
    }
    output_layer.mutate(rng, mutation_rate);
  }

  void decay_random(std::mt19937 &rng, float decay_probability) {
    for (auto &layer : hidden_layers) {
      layer.decay_random(rng, decay_probability);
    }
    output_layer.decay_random(rng, decay_probability);
  }

  void clamp_parameters() {
    for (auto &layer : hidden_layers) {
      layer.clamp_parameters();
    }
    output_layer.clamp_parameters();
  }

  std::string get_shape() const {
    std::string shape = "Pow2NeuralNet: ";

    if (hidden_layers.empty()) {
      shape += std::to_string(output_layer.inputs) + "x" + std::to_string(output_layer.outputs);
      return shape;
    }

    // First hidden layer
    shape +=
        std::to_string(hidden_layers[0].inputs) + "x" + std::to_string(hidden_layers[0].outputs);

    // Remaining hidden layers
    for (size_t i = 1; i < hidden_layers.size(); ++i) {
      shape += " -> " + std::to_string(hidden_layers[i].inputs) + "x" +
               std::to_string(hidden_layers[i].outputs);
    }

    // Output layer
    shape +=
        " -> " + std::to_string(output_layer.inputs) + "x" + std::to_string(output_layer.outputs);

    return shape;
  }

  std::vector<ParamSpan> get_spans() {
    std::vector<ParamSpan> spans;
    for (auto &layer : hidden_layers) {
      auto layer_spans = layer.get_spans();
      spans.insert(spans.end(), layer_spans.begin(), layer_spans.end());
    }
    auto output_spans = output_layer.get_spans();
    spans.insert(spans.end(), output_spans.begin(), output_spans.end());
    return spans;
  }
};

} // namespace model
