#pragma once
#include <random>
#include "../core.h"
#include "../neural_net.h"  // Include file with your trinary neural net definition
#include "../interfaces.h"

namespace jnb {

class MLPTriModel : public Model {
public:
    MLPTriModel(std::mt19937 &rng);
    ~MLPTriModel() = default;
    PlayerInput forward(const GameState &state, bool p1_perspective) override;
    void mutate(std::mt19937 &rng, float mutation_rate) override;
    void reset() override;
    std::shared_ptr<Model> clone() const override;
    std::string get_name() const override;
private:
    // Use your trinary neural net implementation
    StaticNeuralNetTrinary<float, SIMPLE_INPUT_COUNT, 32, 2, SIMPLE_OUTPUT_COUNT> net{};
};

} // namespace jnb