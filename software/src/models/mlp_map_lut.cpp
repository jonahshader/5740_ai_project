#include "mlp_map_lut.h"

#include <cassert>

namespace model {

// constructor just passes parameters and the base model.
// actual initialization happens in init()
SimpleModelTileEmb::SimpleModelTileEmb(size_t map_width_tiles, size_t map_height_tiles,
                                       size_t embedding_vec_size, size_t embedding_coord_count,
                                       bool separate_embeddings_per_coord,
                                       std::shared_ptr<Model<obs::Simple>> base_model)
    : map_width_tiles(map_width_tiles), map_height_tiles(map_height_tiles),
      embedding_vec_size(embedding_vec_size), embedding_coord_count(embedding_coord_count),
      separate_embeddings_per_coord(separate_embeddings_per_coord), base_model(base_model) {}

void SimpleModelTileEmb::forward(const obs::TileCoords &observation, std::vector<float> &action) {
  assert(embedding_coord_count == observation.coords.size());

  // copy observation into observation_with_embeddings
  observation_with_embeddings.clear();
  observation_with_embeddings.reserve(observation.simple.size() +
                                      embedding_vec_size * embedding_coord_count);
  observation_with_embeddings.insert(observation_with_embeddings.end(), observation.simple.begin(),
                                     observation.simple.end());

  // populate the remainder from the embeddings
  for (size_t i = 0; i < observation.coords.size(); ++i) {
    auto &coord = observation.coords[i];
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

void SimpleModelTileEmb::init(const obs::TileCoords &sample_observation, size_t output_size, std::mt19937 &rng) {
  // init base model
  size_t total_input_size = sample_observation.simple.size() + embedding_vec_size * embedding_coord_count;
  // base model's init expects a sample observation, which is a vector<float>.
  // make one with the correct size and pass it
  std::vector<float> base_model_sample_obs;
  base_model_sample_obs.resize(total_input_size);
  base_model->init(base_model_sample_obs, output_size, rng);
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

} // namespace model
