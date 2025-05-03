#include "mlp_map_lut.h"

#include <cassert>

namespace model {

// constructor just passes parameters and the base model.
// actual initialization happens in init()
SimpleModelTileEmb::SimpleModelTileEmb(size_t map_width_tiles, size_t map_height_tiles,
                                       size_t embedding_vec_size, size_t embedding_coord_count,
                                       bool separate_embeddings_per_coord,
                                       std::shared_ptr<SimpleModel> base_model)
    : map_width_tiles(map_width_tiles), map_height_tiles(map_height_tiles),
      embedding_vec_size(embedding_vec_size), embedding_coord_count(embedding_coord_count),
      separate_embeddings_per_coord(separate_embeddings_per_coord), base_model(base_model) {}

void SimpleModelTileEmb::forward(const std::vector<std::pair<int, int>> &lut_coords,
                                 const std::vector<float> &observation,
                                 std::vector<float> &action) {
  assert(embedding_coord_count == lut_coords.size());

  // copy observation into observation_with_embeddings
  observation_with_embeddings.clear();
  observation_with_embeddings.reserve(observation.size() +
                                      embedding_vec_size * embedding_coord_count);
  observation_with_embeddings.insert(observation_with_embeddings.end(), observation.begin(),
                                     observation.end());

  // populate the remainder from the embeddings
  for (size_t i = 0; i < lut_coords.size(); ++i) {
    auto &coord = lut_coords[i];
    // get the embedding for this coordinate
    if (separate_embeddings_per_coord) {
      embeddings[i].get(observation_with_embeddings, coord.first, coord.second);
    } else {
      embeddings[0].get(observation_with_embeddings, coord.first, coord.second);
    }
  }

  // forward pass through the base model
  // the base model takes the observation + embedding as input
  // and populates the action vector
  base_model->forward(observation_with_embeddings, action);
}

void SimpleModelTileEmb::mutate(std::mt19937 &rng, float mutation_rate) {
  // mutate the base model and the embeddings
  base_model->mutate(rng, mutation_rate);
  for (auto &embedding : embeddings) {
    embedding.mutate(rng, mutation_rate);
  }
}

void SimpleModelTileEmb::reset() {
  // embeddings are not stateful, but the base model could be
  base_model->reset();
}

void SimpleModelTileEmb::init(size_t input_size, size_t output_size, std::mt19937 &rng) {
  // init base model
  size_t total_input_size = input_size + embedding_vec_size * embedding_coord_count;
  base_model->init(total_input_size, output_size, rng);
  observation_with_embeddings.reserve(total_input_size);

  // init embeddings
  embeddings.clear();
  // always create at least one embedding
  embeddings.emplace_back(TileEmbeddings{});
  embeddings.back().init(map_width_tiles, map_height_tiles, embedding_vec_size, rng);
  // if we have separate embeddings per coord, add the remaining embeddings
  if (separate_embeddings_per_coord) {
    // create one embedding per coordinate, excluding the first one we made
    for (size_t i = 1; i < embedding_coord_count; ++i) {
      embeddings.emplace_back(TileEmbeddings{});
      embeddings.back().init(map_width_tiles, map_height_tiles, embedding_vec_size, rng);
    }
  }
}

std::shared_ptr<Model> SimpleModelTileEmb::clone() const {
  // make a copy with the built-in copy constructor.
  auto clone = std::make_shared<SimpleModelTileEmb>(*this);
  // since the copy constructor copies by value, that means the shared_ptr of
  // the base model is copied, not the model itself, so I need to clone that here:
  clone->base_model = std::dynamic_pointer_cast<SimpleModel>(clone->base_model->clone());
  return clone;
}

std::string SimpleModelTileEmb::get_name() const {
  return "SimpleModelTileEmb";
}

} // namespace model
