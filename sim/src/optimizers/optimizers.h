#pragma once

#include <limits>
#include <optional>

#include "core.h"

template <typename T>
struct Solution {
  T sol;
  std::optional<int> fitness;
};

template <typename T>
using Population = std::vector<Solution<T>>;

