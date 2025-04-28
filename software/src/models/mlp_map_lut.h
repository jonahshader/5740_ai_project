#pragma once

#include <random>
#include <vector>

#include "jnb.h"
#include "neural_net.h"
#include "interfaces.h"

namespace jnb {

struct TileEmbeddings {
  int width;
  int height;
  int channels;
  std::vector<float> data;

  void init(int width, int height, int channels, std::mt19937 &rng) {
    this->width = width;
    this->height = height;
    this->channels = channels;
    // allocate
    data.resize(width * height * channels);
    // xavier/glorot init
    float stddev = std::sqrt(2.0f / (channels));
    std::normal_distribution<float> dist(0.0f, stddev);
    for (auto &val : data) {
      val = dist(rng);
    }
  }

  void get(std::vector<float> &input, int tile_x, int tile_y) {
    tile_x = std::max(0, std::min(tile_x, width - 1));
    tile_y = std::max(0, std::min(tile_y, height - 1));
    int index = (tile_y * width + tile_x) * channels;
    for (int i = 0; i < channels; ++i) {
      input.push_back(data[index + i]);
    }
  }

  void mutate(std::mt19937 &rng, float mutation_rate) {
    // xavier/glorot initialization
    float stddev = std::sqrt(2.0f / (channels));
    std::normal_distribution<float> dist(0.0f, stddev * mutation_rate);
    for (auto &val : data) {
      val += dist(rng);
    }
  }
};

class MLPMapLutModel : public Model {
public:
  MLPMapLutModel(std::mt19937 &rng, int map_width_tiles, int map_height_tiles);
  ~MLPMapLutModel() = default;

  PlayerInput forward(const GameState &state, bool p1_perspective) override;
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void reset() override;
  std::shared_ptr<Model> clone() const override;
  std::string get_name() const override;

private:
  DynamicNeuralNet<float> net{};
  TileEmbeddings player_embeddings{};
  TileEmbeddings coin_embeddings{};
};

} // namespace jnb
