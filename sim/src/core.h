#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "fixed_point.h"
#include "parse_map.h"

namespace jnb {

using F4 = FixedPoint<int16_t, 4>;
constexpr F4 F4_ZERO = F4::from_raw(0);

constexpr int CELL_SIZE = 8;
constexpr int PLAYER_WIDTH = CELL_SIZE - 2;
constexpr int PLAYER_HEIGHT = CELL_SIZE - 2;
constexpr int PLAYER_KILL_HEIGHT = PLAYER_HEIGHT / 2;

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
  TileMap map{};
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

GameState init(const std::string &map_filename, uint64_t seed);
void update(GameState &state, const PlayerInput &in1, const PlayerInput &in2);
void observe_state_simple(const GameState &state, std::vector<F4> &observation);
void observe_state_screen(const GameState &state, std::vector<uint8_t> &observation);

} // namespace jnb
