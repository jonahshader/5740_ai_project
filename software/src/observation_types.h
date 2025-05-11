#pragma once

#include <functional>
#include <utility>
#include <vector>

namespace obs {

using Simple = std::vector<float>;

struct TileCoords {
  Simple simple{};
  std::vector<std::pair<int, int>> coords{};
};

// TODO: image observation type
// also need a template type to describe the shape of the observation, which
// is passed to the model init method.

} // namespace obs
