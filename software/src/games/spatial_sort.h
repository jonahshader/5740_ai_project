#pragma once

#include <random>
#include <span>
#include <utility>
#include <vector>

#include "game.h"
#include "nn_utils.h"
#include "observation_types.h"

class SpatialSort : public Game<obs::Simple> {
public:
  // enum OutOfBoundsBehav { ZERO, FIXED_RANDOM };
  // TODO: add config option to ablate the simulation
  struct Config {
    int key_size{4};    // size of one-hot vector that is to be sorted
    int value_size{6};  // the size of the one-hot payload that is associated with the key
    int item_count{4};  // number of key-value items to be sorted. must be <= key_size for now
    int item_stride{2}; // world_height is item_stride * (item_count + 1) - 1
    float travel_dist_ratio{1.0f}; // world_width is world_height * travel_dist_ratio
    float time_ratio{2.0f};        // simulation_time = max(world_width, world_height) * time_ratio
    int latent_size{16}; // initial zeros concatenated to channel_dim and are untouched by IO
    bool keep_latent_on_inputs{false}; // allow input cells' latent space to change, or clear them

    int fourier_features{4};
    float fourier_std{2.0f};
  };

  struct Item {
    int key;
    int value;
  };

  explicit SpatialSort(const Config &config);

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
  std::vector<float> buffer_read{}, buffer_write{};
  std::vector<Item> inputs{};
  std::vector<Item> outputs{};
  int timestep{0};
  int world_width{0};
  int world_height{0};
  int channel_size{0};
  int agent_x{0};
  int agent_y{0};

  std::mt19937 rng{0};

  void inject_inputs(std::vector<float> &buffer);
};
