#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <utility>

#include "fixed_point.h"

namespace jnb {

using Fixed3 = FixedPoint<int16_t, 4>;

constexpr int CELL_SIZE = 8;
constexpr int PLAYER_WIDTH = CELL_SIZE - 2;
constexpr int PLAYER_HEIGHT = 2 * CELL_SIZE - 2;
constexpr int PLAYER_HALF_HEIGHT = PLAYER_HEIGHT / 2;
constexpr int WIDTH_CELLS = 10;
constexpr int HEIGHT_CELLS = 10;

using TilePos = std::pair<uint8_t, uint8_t>;

// clang-format off
// note: this is using y-down indexing,
// so 0, 0 is top left, HEIGHT_CELLS-1, 0 is bottom left.
constexpr uint8_t base_map[HEIGHT_CELLS][WIDTH_CELLS] = {
  {2,2,2,2,2,2,2,2,2,2},
  {2,2,2,1,1,2,2,1,1,2},
  {1,2,2,2,2,2,2,1,1,2},
  {1,1,2,2,2,2,2,2,2,2},
  {2,2,2,2,2,2,2,2,2,2},
  {2,2,2,1,1,3,1,1,2,2},
  {2,2,2,2,2,2,2,2,2,2},
  {2,2,2,2,2,2,2,2,2,1},
  {5,6,6,6,6,6,5,5,1,1},
  {4,4,4,4,4,4,4,4,1,1},
};
// clang-format on

enum TILE {
  NOTHING = 0,
  GROUND = 1,
  AIR = 2,
  SPRING = 3,
  WATER_BODY = 4,
  WATER_TOP = 5,
  ICE = 6,
  COIN = 7
};

// we want to spawn coins in the air, but above non-air (so that they are accessible).
// since the map is know at compile time, we can generate a list of these spawn locations
// at compile time as well.

// counts the number of valid coin spawn locations
// for FPGA implementation, this can be cached
constexpr int count_coin_spawns() {
  int count = 0;
  for (uint8_t y = 1; y < HEIGHT_CELLS; ++y) { // exclude top row
    for (uint8_t x = 0; x < WIDTH_CELLS; ++x) {
      if (base_map[y][x] == AIR && base_map[y - 1][x] != AIR) {
        ++count;
      }
    }
  }
  return count;
}

// get the positions matching our criteria
template <int Count> constexpr auto get_coin_spawns() {
  std::array<TilePos, Count> positions{};
  int index = 0;

  for (uint8_t y = 1; y < HEIGHT_CELLS; ++y) {
    for (uint8_t x = 0; x < WIDTH_CELLS; ++x) {
      if (base_map[y][x] == AIR && base_map[y - 1][x] != AIR) {
        positions[index] = {x, y};
        ++index;
      }
    }
  }

  return positions;
}

// determine the number of coin spawn locations
constexpr int COIN_SPAWN_COUNT = count_coin_spawns();
// generate the coin spawn locations
constexpr auto COIN_SPAWN_POSITIONS = get_coin_spawns<COIN_SPAWN_COUNT>();

struct Player {
  Fixed3 x;
  Fixed3 y;
  Fixed3 x_vel;
  Fixed3 y_vel;
};

struct GameState {
  Player p1;
  Player p2;
  TilePos coin_pos;
  std::mt19937 rng;
};

struct PlayerInput {
  bool left;
  bool right;
  bool jump;
};

void update(GameState &state, const PlayerInput &in1, const PlayerInput &in2);

} // namespace jnb
