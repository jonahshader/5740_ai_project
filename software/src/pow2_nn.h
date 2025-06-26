#pragma once

#include <cassert>
#include <functional>
#include <random>
#include <span>
#include <vector>

#include "model.h"
#include "nn_utils.h"

namespace model {

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
  std::vector<int> weights; // power values
  std::vector<int> bias;    // direct values
  int pre_act_shift_right;

  explicit Pow2Layer(ActFun act_fun = ActFun{}) : act_fun(act_fun) {}

  int get_w(size_t input, size_t output) {
    return weights[output * inputs + input];
  }

  void set_w(size_t input, size_t output, int value) {
    weights[output * inputs + input] = value;
  }

  void init(std::mt19937 &rng, size_t inputs, size_t outputs, size_t weight_max_abs,
            size_t bias_max_abs, int pre_act_shift_right_min, int pre_act_shift_left_min) {
    // allocate memory
    this->inputs = inputs;
    this->outputs = outputs;
    this->weight_max_abs = weight_max_abs;
    this->bias_max_abs = bias_max_abs;
    this->pre_act_shift_right_min = pre_act_shift_right_min;
    this->pre_act_shift_right_max = pre_act_shift_left_min;
    weights.resize(inputs * outputs);
    bias.resize(outputs);

    // xavier/glorot init
    float stddev = std::sqrt(2.0f / (inputs + outputs));

    std::bernoulli_distribution sign_dist(0.5f);

    // use triangular distribution to make the dist bounded
    float radius = stddev * std::sqrt(6.0f);
    unsigned int expand_scale = std::pow(2, weight_max_abs) / radius;
    pre_act_shift_right = std::ceil(std::log2(expand_scale));

    for (auto i = 0; i < outputs; ++i) {
      for (auto j = 0; j < inputs; ++j) {
        auto triangle_sample = sample_half_triangle_dist(std::powf(2, weight_max_abs));
        // round to the nearest power of two
        // TODO: do some sort of sigma-delta modulation instead of rounding,
        // or do stochastic dithering/rounding? current solution might lead
        // to smaller overall variance. i suppose i could also just calculate the
        // variance and correct for it by adjusting pre_act_shift_right,
        // but both are powers of two... numerator (aka weights) need to be stochastically
        // selected to average out to a desired variance that can be corrected with a
        // shift right.
        int triangle_sample_pow = std::round(std::log2(triangle_sample));
        set_w(j, i, sign_dist(rng) ? triangle_sample_pow : -triangle_sample_pow);

        // TODO: experiment with making mutation probabilities inversely proportional to current
        // weight value. the intuition is that this would counter-act the increase in deltas as
        // weight values get higher.
        // e.g., going from w=5 to w=6 is going from 2^5 to 2^6 = delta of 32, so the probability
        // of the mutation occurring should be
        // scaled by 1/32 ish (probably the average of (1/16 + 1/32)/2).
      }
      bias[i] = 0; // no bias by default
    }
  }

  void forward(const std::span<int> input, std::span<int> output) {
    assert(input.size() >= inputs);
    assert(output.size() >= outputs);
    for (auto i = 0; i < outputs; ++i) {
      output[i] = bias[i];
      for (auto j = 0; j < inputs; ++j) {
        auto w = get_w(j, i);
        output[i] += w >= 0 ? input[j] << w : (-input[j]) << (-w);
      }
      // scale down output
      output[i] >>= pre_act_shift_right;
      // activate
      output[i] = act_fun(output[i]);
    }
  }
};

} // namespace model
