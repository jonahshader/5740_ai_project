#pragma once

#include <random>
#include <memory>
#include <vector>

#include "jnb.h"
#include "neural_net.h"
#include "model.h"

namespace model {

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

class SimpleModelTileEmb : public Model {
public:
  SimpleModelTileEmb(size_t map_width_tiles, size_t map_height_tiles, size_t embedding_vec_size,
                     size_t embedding_coord_count, bool separate_embeddings_per_coord,
                     std::shared_ptr<SimpleModel> base_model);
  ~SimpleModelTileEmb() = default;

  void forward(const std::vector<std::pair<int, int>> &lut_coords,
               const std::vector<float> &observation, std::vector<float> &action);
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void reset() override;
  void init(size_t input_size, size_t output_size, std::mt19937 &rng) override;
  std::shared_ptr<Model> clone() const override;
  std::string get_name() const override;

private:
  size_t map_width_tiles;
  size_t map_height_tiles;
  size_t embedding_vec_size;
  size_t embedding_coord_count;
  bool separate_embeddings_per_coord;
  std::shared_ptr<SimpleModel> base_model{};
  std::vector<TileEmbeddings> embeddings{};
  std::vector<float> observation_with_embeddings{};
};

} // namespace model
