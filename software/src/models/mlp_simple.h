#pragma once

#include <random>

#include "jnb.h"
#include "model.h"
#include "neural_net.h"
#include "observation_types.h"

namespace model {

// TODO: expose  DynamicNeuralNet datatype as template T?
class SimpleMLP : public Model<obs::Simple> {
public:
  SimpleMLP(size_t hidden_size, size_t hidden_count);
  ~SimpleMLP() = default;
  void forward(const obs::Simple &observation, std::vector<float> &action) override {
    net.forward(observation.data(), action.data());
  }
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void init(const obs::Simple &sample_observation, size_t output_size, std::mt19937 &rng) override {
    net.init(rng, sample_observation.size(), hidden_size, hidden_count, output_size);
  }
  std::shared_ptr<Model<obs::Simple>> clone() const override {
    return std::make_shared<SimpleMLP>(*this);
  }
  std::string get_name() const override {
    return "SimpleMLP";
  }

private:
  size_t hidden_size;
  size_t hidden_count;
  DynamicNeuralNet<float> net{};
};

} // namespace model
