#pragma once

#include <random>

#include "jnb.h"
#include "model.h"
#include "neural_net.h"
#include "nn_utils.h"
#include "observation_types.h"
#include "pow2_nn.h"

namespace model {

// Activation functions for Pow2 neural network
inline auto relu_activation = [](auto x) {
  // hidden activation: relu
  using T = decltype(x);
  return std::max(x, T{0});
};

inline auto identity_activation = [](auto x) {
  // output activation: identity
  return x;
};

// TODO: expose  DynamicNeuralNet datatype as template T?
class SimpleMLP : public Model<obs::Simple> {
public:
  SimpleMLP(size_t hidden_size, size_t hidden_count);
  void forward(const obs::Simple &observation, std::vector<float> &action) override {
    net.forward(observation.data(), action.data());
  }
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void init(const obs::Simple &sample_observation, size_t output_size, std::mt19937 &rng) override {
    net.init(rng, sample_observation.size(), hidden_size, hidden_count, output_size);
  }
  std::shared_ptr<BaseModel> base_clone() const override {
    return std::make_shared<SimpleMLP>(*this);
  }
  std::shared_ptr<Model<obs::Simple>> clone() const override {
    return std::make_shared<SimpleMLP>(*this);
  }
  std::string get_name() const override {
    return "SimpleMLP";
  }
  std::vector<ParamSpan> get_spans() override;
  void apply_spans() override;

private:
  size_t hidden_size;
  size_t hidden_count;
  DynamicNeuralNet<float> net{};
};

class SimplePow2MLP : public Model<obs::Simple> {
public:
  SimplePow2MLP(size_t hidden_size, size_t hidden_count);
  void forward(const obs::Simple &observation, std::vector<float> &action) override {
    inputs_ints.resize(observation.size());
    actions_ints.resize(action.size());
    
    for (size_t i = 0; i < observation.size(); ++i) {
      inputs_ints[i] = static_cast<int>(std::round(observation[i] * 256.0f));
    }

    net.forward(inputs_ints, actions_ints);
    
    for (size_t i = 0; i < action.size(); ++i) {
      action[i] = static_cast<float>(actions_ints[i]) / 256.0f;
    }
  }
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void init(const obs::Simple &sample_observation, size_t output_size, std::mt19937 &rng) override {
    net.init(rng, sample_observation.size(), hidden_size, hidden_count, output_size, 4, 1000, 0, 8, relu_activation, identity_activation);
  }
  std::shared_ptr<BaseModel> base_clone() const override {
    return std::make_shared<SimplePow2MLP>(*this);
  }
  std::shared_ptr<Model<obs::Simple>> clone() const override {
    return std::make_shared<SimplePow2MLP>(*this);
  }
  std::string get_name() const override {
    return "SimplePow2MLP";
  }
  std::vector<ParamSpan> get_spans() override;
  void apply_spans() override;

private:
  size_t hidden_size;
  size_t hidden_count;
  Pow2NeuralNet<decltype(relu_activation), decltype(identity_activation)> net{};

  std::vector<int> inputs_ints{};
  std::vector<int> actions_ints{};
};

} // namespace model
