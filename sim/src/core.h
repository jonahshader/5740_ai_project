#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

#include "fixed_point.h"

namespace jnb {

using Fixed3 = FixedPoint<int16_t, 3>;

constexpr int CELL_SIZE = 8;
constexpr int PLAYER_WIDTH = CELL_SIZE - 2;
constexpr int PLAYER_HEIGHT = CELL_SIZE - 2;
constexpr int PLAYER_HALF_HEIGHT = PLAYER_HEIGHT / 2;
constexpr int WIDTH_CELLS = 10;
constexpr int HEIGHT_CELLS = 10;

constexpr Fixed3 JUMP_VEL = Fixed3::from_raw(12);
constexpr Fixed3 JUMP_MIDAIR_ACCEL = Fixed3::from_raw(1);
constexpr Fixed3 SPRING_VEL = Fixed3::from_raw(18);
constexpr Fixed3 MOVE_ACCEL = Fixed3::from_raw(3);
constexpr Fixed3 MOVE_ACCEL_WATER = Fixed3::from_raw(2); // slower in water
constexpr Fixed3 MOVE_ACCEL_ICE = Fixed3::from_raw(1);   // much slower on ice
constexpr Fixed3 MOVE_MAX_VEL = Fixed3((int16_t)2);      // 2 pixels per second
constexpr Fixed3 GRAVITY = Fixed3::from_raw(-3);
constexpr Fixed3 GRAVITY_WATER = Fixed3::from_raw(-1);

struct TilePos {
  uint8_t x;
  uint8_t y;
};

enum Tile {
  NOTHING = 0,
  GROUND = 1,
  AIR = 2,
  SPRING = 3,
  WATER_BODY = 4,
  WATER_TOP = 5,
  ICE = 6,
  COIN = 7
};

constexpr bool is_solid(Tile tile) {
  return tile == GROUND || tile == SPRING || tile == ICE;
}

constexpr bool is_water(Tile tile) {
  return tile == WATER_BODY || tile == WATER_TOP;
}

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
  int score;
};

struct GameState {
  Player p1;
  Player p2;
  TilePos coin_pos;
  std::mt19937 rng;
  uint32_t age;
};

struct PlayerInput {
  bool left;
  bool right;
  bool jump;
};

void update(GameState &state, const PlayerInput &in1, const PlayerInput &in2);
void observe_state_simple(const GameState &state, std::vector<Fixed3> &observation);
void observe_state_screen(const GameState &state, std::vector<uint8_t> &observation);

} // namespace jnb
