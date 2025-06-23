#pragma once

#include <random>
#include <utility>
#include <vector>

#include "game.h"
#include "nn_utils.h"
#include "observation_types.h"

// class DiscreteSeek : public Game<obs::Simple> {
// public:
//   struct Config {
//     size_t world_width{32};
//     size_t world_height{32};
//     int frame_limit{40};
//     bool absolute_inputs{true};
//     bool delta_inputs{true};
//     std::vector<FloatToVec> fourier_transforms{};
//   };

//   explicit DiscreteSeek(const Config &config);

//   void init(uint64_t seed) override;
//   void update(const std::vector<std::vector<float>> &actions);
//   void get_fitness(std::vector<int32_t> &fitness) override;
//   bool is_done() override;
//   void observe(std::vector<obs::Simple> &inputs) override;
//   size_t get_action_count() override;
//   size_t get_player_count() override;
//   std::string get_name() override;
//   void render(std::vector<uint32_t> &pixels) override;
//   std::pair<int, int> get_resolution() override;

// private:
//   Config config;

//   // game state
//   int x_coin{0};
//   int y_coin{0};
//   int x{0};
//   int y{0};
//   std::vector<bool> map{};

//   // resources
//   std::shared_ptr<std::vector<uint8_t>> spritesheet{nullptr};
// };

class ContinuousSeek : public Game<obs::Simple> {
public:
  struct Config {
    float world_width{32.0f};
    float world_height{32.0f};
    float coin_radius{1.5f};
    float friction{6.0f};
    float max_accel{100.0f};
    float dt{1 / 15.0f};
    int frame_limit{300};
    bool absolute_inputs{true};
    bool relative_inputs{true};
    std::vector<FloatToVec> fourier_transforms{};
  };

  explicit ContinuousSeek(const Config &config);

  void init(uint64_t seed) override;
  void update(const std::vector<std::vector<float>> &actions) override;
  void get_fitness(std::vector<int32_t> &fitness) override;
  bool is_done() override;
  void observe(std::vector<obs::Simple> &inputs) override;
  size_t get_action_count() override;
  size_t get_player_count() override;
  std::string get_name() override;
  void render(std::vector<uint32_t> &pixels) override;
  std::pair<int, int> get_resolution() override;
  std::unique_ptr<Game<obs::Simple>> clone() const override;

private:
  Config config;
  float x{0.0f};
  float y{0.0f};
  float x_vel{0.0f};
  float y_vel{0.0f};
  size_t coins_collected{0};

  float x_coin{0.0f};
  float y_coin{0.0f};
  float px_coin{0.0f};
  float py_coin{0.0f};
  int timestep{0};

  std::mt19937 rng{0};
};
