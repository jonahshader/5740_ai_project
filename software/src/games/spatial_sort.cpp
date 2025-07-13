#include "spatial_sort.h"

#include <algorithm>
#include <cassert>

SpatialSort::SpatialSort(const Config &config) : config(config) {}

void SpatialSort::init(uint64_t seed) {
  timestep = 0;
  rng = std::mt19937(seed);
  agent_x = 0;
  agent_y = 0;

  // determine world size, channel size, buffer size
  world_height = config.item_stride * (config.item_count + 1) - 1;
  world_width = config.travel_dist_ratio * world_height;
  channel_size = config.key_size + config.value_size + config.latent_size;
  size_t buffer_size = static_cast<size_t>(world_width) * world_height * channel_size;
  buffer_read.resize(buffer_size);
  buffer_write.resize(buffer_size);
  // init buffers to zeros
  std::fill(buffer_read.begin(), buffer_read.end(), 0.0f);
  std::fill(buffer_write.begin(), buffer_write.end(), 0.0f);

  inputs.clear();
  outputs.clear();
  inputs.reserve(config.item_count);
  outputs.reserve(config.item_count);

  // TODO: this is wrong. need to do shuffling on two separate key and value vecs,
  // then use that to build the item vecs and then sort outputs.
  std::uniform_int_distribution<int> key_dist(0, config.key_size - 1);
  std::uniform_int_distribution<int> value_dist(0, config.value_size - 1);
  for (int i = 0; i < config.item_count; ++i) {
    Item item{key_dist(rng), value_dist(rng)};
    inputs.push_back(item);
    outputs.push_back(item); // initially, outputs are the same as inputs
  }

  // sort outputs by key
  // (this is the operation that we are training the network to do)
  std::sort(outputs.begin(), outputs.end(),
            [](const Item &a, const Item &b) { return a.key < b.key; });

  inject_inputs(buffer_read);
}

void SpatialSort::inject_inputs(std::vector<float> &buffer) {
  for (int item_index = 0; item_index < inputs.size(); ++item_index) {
    const Item &item = inputs[item_index];
    int y = config.item_stride * (inputs.size() + 1) - 1;
    for (int c = 0; c < config.key_size; ++c) {
      buffer[y * world_width * channel_size + c] = c == item.key ? 1.0f : 0.0f;
    }
    for (int c = config.key_size; c < config.key_size + config.value_size; ++c) {
      buffer[y * world_width * channel_size + c] =
          (c - config.key_size) == item.value ? 1.0f : 0.0f;
    }
    if (!config.keep_latent_on_inputs) {
      for (int c = config.key_size + config.value_size; c < channel_size; ++c) {
        buffer[y * world_width * channel_size + c] = 0.0f; // reset latent
      }
    }
  }
}

void SpatialSort::update(const std::vector<std::vector<float>> &actions) {
  // copy actions directly into write cell
  assert(actions.size() == get_player_count()); // ensure correct player count
  assert(actions[0].size() == channel_size);    // ensure correct action size

  // determine buffer write start location
  int write_start = agent_y * world_width * channel_size + agent_x * channel_size;
  // copy directly into write buffer
  std::copy(actions[0].begin(), actions[0].end(), buffer_write.begin() + write_start);

  ++agent_x;
  if (agent_x >= world_width) {
    agent_x = 0;
    ++agent_y;
  }
  if (agent_y >= world_height) {
    agent_y = 0;
    ++timestep;
    // swap buffers
    std::swap(buffer_read, buffer_write);
    // inject inputs into the read buffer
    inject_inputs(buffer_read);
  }
}

void SpatialSort::get_fitness(std::vector<int32_t> &fitness) {
  fitness.resize(get_player_count());

  // TODO: iterate through and measure how close the values are
}

bool SpatialSort::is_done() {
  int frame_limit = config.time_ratio * std::max(world_width, world_height);
  return timestep >= frame_limit and frame_limit >= 0;
}

void SpatialSort::observe(std::vector<obs::Simple> &inputs) {}

size_t SpatialSort::get_action_count() {
  return config.key_size + config.value_size + config.latent_size;
}

size_t SpatialSort::get_player_count() {
  return 1; // single player
}

std::string SpatialSort::get_name() {
  return "SpatialSort";
}

void SpatialSort::render(std::vector<uint32_t> &pixels) {}

std::pair<int, int> SpatialSort::get_resolution() {
  // scale up arbitrarily
  return {static_cast<int>(world_width * 8), static_cast<int>(world_height * 8)};
}

std::unique_ptr<Game<obs::Simple>> SpatialSort::clone() const {
  return std::make_unique<SpatialSort>(*this);
}
