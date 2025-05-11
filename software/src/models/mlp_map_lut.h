#pragma once

#include <random>
#include <memory>
#include <vector>

#include "jnb.h"
#include "neural_net.h"
#include "model.h"
#include "observation_types.h"

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

class SimpleModelTileEmb : public Model<obs::TileCoords> {
public:
  SimpleModelTileEmb(size_t map_width_tiles, size_t map_height_tiles, size_t embedding_vec_size,
                     size_t embedding_coord_count, bool separate_embeddings_per_coord,
                     std::shared_ptr<Model<obs::Simple>> base_model);
  ~SimpleModelTileEmb() = default;

  void forward(const obs::TileCoords &observation, std::vector<float> &action);
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void init(const obs::TileCoords &sample_observation, size_t output_size,
            std::mt19937 &rng) override;
  void reset() override {
    // embeddings are not stateful, but the base model could be
    base_model->reset();
  }
  std::shared_ptr<Model<obs::TileCoords>> clone() const override {
    // make a copy with the built-in copy constructor
    auto clone = std::make_shared<SimpleModelTileEmb>(*this);
    // since the copy constructor copies by value, that means the shared_ptr of
    // the base model is copied, not the model itself, so I need to clone that here:
    clone->base_model = clone->base_model->clone();
    return clone;
  }
  std::string get_name() const override {
    return "SimpleModelTileEmb";
  }

private:
  size_t map_width_tiles;
  size_t map_height_tiles;
  size_t embedding_vec_size;
  size_t embedding_coord_count;
  bool separate_embeddings_per_coord;
  std::shared_ptr<Model<obs::Simple>> base_model{};
  std::vector<TileEmbeddings> embeddings{};
  std::vector<float> observation_with_embeddings{};
};

} // namespace model
