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

using Image = std::vector<std::vector<std::uint8_t>>;

} // namespace obs
