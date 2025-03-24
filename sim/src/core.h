#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

#include "fixed_point.h"

namespace jnb {

using F4 = FixedPoint<int16_t, 4>;
constexpr F4 F4_ZERO = F4::from_raw(0);

constexpr int CELL_SIZE = 8;
constexpr int PLAYER_WIDTH = CELL_SIZE - 2;
constexpr int PLAYER_HEIGHT = CELL_SIZE - 2;
constexpr int PLAYER_KILL_HEIGHT = PLAYER_HEIGHT / 2;
constexpr int WIDTH_CELLS = 10;
constexpr int HEIGHT_CELLS = 10;

constexpr int POINTS_PER_COIN = 3;
constexpr int POINTS_PER_KILL = 1;
constexpr int DEAD_TIMEOUT = 60; // in frames

constexpr F4 JUMP_VEL = F4::from_raw(25);
constexpr F4 JUMP_MIDAIR_ACCEL = F4::from_raw(1);
constexpr F4 SPRING_VEL = F4::from_raw(38);
constexpr F4 MOVE_ACCEL = F4::from_raw(3);
constexpr F4 MOVE_ACCEL_WATER = F4::from_raw(2); // slower in water
constexpr F4 MOVE_ACCEL_ICE = F4::from_raw(1);   // much slower on ice
constexpr F4 MOVE_MAX_VEL = F4::from_raw(10);
constexpr F4 GRAVITY = F4::from_raw(-2);
constexpr F4 GRAVITY_WATER = F4::from_raw(-1);
constexpr F4 FALL_MAX_VEL = F4::from_raw(-20);

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

constexpr Tile read_base_map(int x, int y) {
  if (x < 0 || y < 0 || x >= WIDTH_CELLS)
    return Tile::GROUND;
  if (y >= HEIGHT_CELLS)
    return Tile::AIR;
  return static_cast<Tile>(base_map[HEIGHT_CELLS - 1 - y][x]);
}

// we want to spawn coins in the air, but above non-air (so that they are accessible).
// since the map is known at compile time, we can generate a list of these spawn locations
// at compile time as well.

// counts the number of valid coin spawn locations
// for FPGA implementation, this can be cached
constexpr int count_coin_spawns() {
  int count = 0;
  for (uint8_t y = 0; y < HEIGHT_CELLS - 1; ++y) { // exclude top row
    for (uint8_t x = 0; x < WIDTH_CELLS; ++x) {
      if (is_solid(static_cast<Tile>(read_base_map(x, y))) && read_base_map(x, y + 1) == AIR) {
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
      if (is_solid(static_cast<Tile>(read_base_map(x, y))) && read_base_map(x, y + 1) == AIR) {
        positions[index] = {x, static_cast<uint8_t>(y + 1)};
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
  F4 x = F4::from_raw(0);
  F4 y = F4::from_raw(0);
  F4 x_vel = F4::from_raw(0);
  F4 y_vel = F4::from_raw(0);
  int score{0};
  int dead_timeout{0};
  bool queue_dead{false}; // this is NOT needed on FPGA, just a hack for CPU version
};

struct GameState {
  Player p1{};
  Player p2{};
  TilePos coin_pos{0, 0};
  std::mt19937 rng{0};
  uint32_t age{0};
};

struct PlayerInput {
  bool left{false};
  bool right{false};
  bool jump{false};
};

GameState init(uint64_t seed);
void update(GameState &state, const PlayerInput &in1, const PlayerInput &in2);
void observe_state_simple(const GameState &state, std::vector<F4> &observation);
void observe_state_screen(const GameState &state, std::vector<uint8_t> &observation);

} // namespace jnb
