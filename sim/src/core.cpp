#include "core.h"

#include <cmath>

namespace jnb {

// read from the base map with y-up indexing,
// out-of-bounds down, left, right returns GROUND.
// out-of-bounds up returns AIR.


TilePos get_random_spawn_pos(std::mt19937 &rng) {
  std::uniform_int_distribution<int> dist(0, COIN_SPAWN_COUNT - 1);
  auto coin_pos_index = dist(rng);
  return COIN_SPAWN_POSITIONS[coin_pos_index];
}

GameState init(uint64_t seed) {
  GameState state{};
  // create rng from seed
  state.rng = std::mt19937(seed);
  // pick random position for coin
  state.coin_pos = get_random_spawn_pos(state.rng);
  // pick random positions for p1, p2
  std::uniform_int_distribution<int> spawn_index_dist(0, COIN_SPAWN_COUNT - 1);
  auto spawn_index = spawn_index_dist(state.rng);
  auto spawn = COIN_SPAWN_POSITIONS[spawn_index];
  state.p1.x = Fixed4(static_cast<int16_t>(spawn.x * CELL_SIZE));
  state.p1.y = Fixed4(static_cast<int16_t>(spawn.y * CELL_SIZE));
  auto spawn_index_2 = spawn_index_dist(state.rng);
  // ensure ps2 doesn't spawn on p1
  // TODO: extend to coin logic? or is this needlessly complicated for FPGA impl?
  if (spawn_index_2 == spawn_index) {
    spawn_index_2 = (spawn_index + 1) % COIN_SPAWN_COUNT;
  }
  spawn = COIN_SPAWN_POSITIONS[spawn_index_2];
  state.p2.x = Fixed4(static_cast<int16_t>(spawn.x * CELL_SIZE));
  state.p2.y = Fixed4(static_cast<int16_t>(spawn.y * CELL_SIZE));

  return state;
}

int get_tile_id(int pos) {
  if (pos >= 0) {
    return pos / CELL_SIZE;
  } else {
    // in our case, just returning -1 here is fine, since the player will never
    // make it past -1 in any case.
    return -1;
  }
}

void update_player(Player &p, const PlayerInput &input, const TilePos &coin_pos,
                   bool &coin_collected) {
  constexpr Fixed4 F3_ZERO = Fixed4::from_raw(0);

  const int x_low = p.x.to_integer_floor();
  const int x_high = p.x.to_integer_ceil();
  const int y_low = p.y.to_integer_floor();
  const int y_high = p.y.to_integer_ceil();

  const int x_tile_left = get_tile_id(x_low);
  const int x_tile_right = get_tile_id(x_high + PLAYER_WIDTH - 1);
  const int y_tile_down = get_tile_id(y_low);
  const int y_tile_up = get_tile_id(y_high + PLAYER_HEIGHT - 1);

  // get the tiles the player is standing on
  const Tile down_left_tile = read_base_map(x_tile_left, y_tile_down - 1);
  const Tile down_right_tile = read_base_map(x_tile_right, y_tile_down - 1);
  const Tile left_tile = read_base_map(x_tile_left, y_tile_down);
  const Tile right_tile = read_base_map(x_tile_right, y_tile_down);

  // determine if the player is grounded
  bool grounded = false;
  if (Fixed4(static_cast<int16_t>(y_tile_down * CELL_SIZE)) == p.y) {
    // we are on the bottom of a tile,
    // so check if we are on something stand-able...
    if (is_solid(down_left_tile) || is_solid(down_right_tile)) {
      grounded = true;
    }
  }

  // determine if in water
  const bool in_water = is_water(left_tile) || is_water(right_tile);
  // determine if on ice
  const bool on_ice = down_left_tile == Tile::ICE || down_right_tile == Tile::ICE;
  // determine acceleration based on context
  const Fixed4 gravity = in_water ? GRAVITY_WATER : GRAVITY;
  const Fixed4 move_accel = on_ice ? MOVE_ACCEL_ICE : (in_water ? MOVE_ACCEL_WATER : MOVE_ACCEL);

  // jump logic
  if (grounded) {
    if (down_left_tile == Tile::SPRING || down_right_tile == Tile::SPRING) {
      p.y_vel = SPRING_VEL;
    } else if (input.jump) {
      p.y_vel = JUMP_VEL;
    }
  } else { // not grounded. accelerate due to gravity
    p.y_vel += gravity;
    // if jump is held, also accelerate up a little to 'float'
    if (input.jump) {
      p.y_vel += JUMP_MIDAIR_ACCEL;
    }
  }

  // accelerate x_vel based on input
  if (input.left && !input.right) {
    // accel left
    p.x_vel -= move_accel;
    // limit
    if (p.x_vel < -MOVE_MAX_VEL) {
      p.x_vel = -MOVE_MAX_VEL;
    }
  } else if (input.right && !input.left) {
    // accel right
    p.x_vel += move_accel;
    // limit
    if (p.x_vel > MOVE_MAX_VEL) {
      p.x_vel = MOVE_MAX_VEL;
    }
  } else {
    // decelerate towards zero if grounded and not on ice
    if (grounded && !on_ice) {
      if (p.x_vel > F3_ZERO) {
        // check if we have room to do the full speed reduction
        if (p.x_vel >= move_accel) {
          p.x_vel -= move_accel;
        } else {
          // we are too slow to do the full speed reduction
          p.x_vel = F3_ZERO;
        }
      } else if (p.x_vel < F3_ZERO) {
        // check if we have room to do the full speed reduction
        if (p.x_vel <= -move_accel) {
          p.x_vel += move_accel;
        } else {
          // we are too slow to do the full speed reduction
          p.x_vel = F3_ZERO;
        }
      }
    }
  }

  // integrate the velocities
  p.x += p.x_vel;
  p.y += p.y_vel;

  // handle collisions against solid tiles
  const int xn_low = p.x.to_integer_floor();
  const int xn_high = p.x.to_integer_ceil();
  const int yn_low = p.y.to_integer_floor();
  const int yn_high = p.y.to_integer_ceil();

  const int xn_tile_left = get_tile_id(xn_low);
  const int xn_tile_right = get_tile_id(xn_high + PLAYER_WIDTH - 1);
  const int yn_tile_down = get_tile_id(yn_low);
  const int yn_tile_up = get_tile_id(yn_high + PLAYER_HEIGHT - 1);

  // handle left right collisions
  if (p.x_vel < F3_ZERO) {
    // going left. check left side
    const Tile left_1 = read_base_map(xn_tile_left, y_tile_down);
    const Tile left_2 = read_base_map(xn_tile_left, y_tile_up);
    if (is_solid(left_1) || is_solid(left_2)) {
      // make flush with wall
      p.x = Fixed4(static_cast<int16_t>((xn_tile_left + 1) * CELL_SIZE));
      // cancel velocity
      p.x_vel = F3_ZERO;
    }
  } else if (p.x_vel > F3_ZERO) {
    // going right. check right side
    const Tile right_1 = read_base_map(xn_tile_right, y_tile_down);
    const Tile right_2 = read_base_map(xn_tile_right, y_tile_up);
    if (is_solid(right_1) || is_solid(right_2)) {
      // make flush with wall
      p.x = Fixed4(static_cast<int16_t>(xn_tile_right * CELL_SIZE - PLAYER_WIDTH));
      // cancel velocity
      p.x_vel = F3_ZERO;
    }
  }
  if (p.y_vel < F3_ZERO) {
    // going down. check bottom
    const Tile down_1 = read_base_map(x_tile_left, yn_tile_down);
    const Tile down_2 = read_base_map(x_tile_right, yn_tile_down);
    if (is_solid(down_1) || is_solid(down_2)) {
      // make flush with floor
      p.y = Fixed4(static_cast<int16_t>((yn_tile_down + 1) * CELL_SIZE));
      // cancel velocity
      p.y_vel = F3_ZERO;
    }
  } else if (p.y_vel > F3_ZERO) {
    // going up. check top
    const Tile top_1 = read_base_map(x_tile_left, yn_tile_up);
    const Tile top_2 = read_base_map(x_tile_right, yn_tile_up);
    if (is_solid(top_1) || is_solid(top_2)) {
      p.y = Fixed4(static_cast<int16_t>(yn_tile_up * CELL_SIZE - PLAYER_HEIGHT));
      // cancel velocity
      p.y_vel = F3_ZERO;
    }
  }

  // check coin collision.
  // can do this in here (which would be both players in parallel in FPGA) because
  // in the edge case where both players collide with the coin on the same frame,
  // we'll just give both of them the point.
  const int x_center = p.x.to_integer_rounded() + PLAYER_WIDTH / 2;
  const int y_center = p.y.to_integer_rounded() + PLAYER_HEIGHT / 2;
  const int x_tile_center = get_tile_id(x_center);
  const int y_tile_center = get_tile_id(y_center);
  coin_collected = x_tile_center == coin_pos.x && y_tile_center == coin_pos.y;
  if (coin_collected) {
    // get a point for collecting coin
    p.score += POINTS_PER_COIN;
  }
}

void update(GameState &state, const PlayerInput &in1, const PlayerInput &in2) {
  // updating can happen in parallel in FPGA
  bool p1_coin_collected;
  bool p2_coin_collected;
  update_player(state.p1, in1, state.coin_pos, p1_coin_collected);
  update_player(state.p2, in2, state.coin_pos, p2_coin_collected);

  // TODO: handle player-player collision.
  // the intent is to make it so that jumping on another player kills them,
  // giving one point to the killer. if they collide but on the side instead
  // of the top, they just collide.
  // actually, maybe this should be handled mario-style, where we compare
  // their y-velocities instead.
  // some other ideas: maybe the player could place a temporary ground tile
  // beneath them, at the expence of one point (the coin reward would have to
  // be much higher, so that its still worth placing ground tiles if it means
  // increased likelyhood of getting the coin)

  // if the coin was collected,
  // pick a new location from the valid coin spawn locations randomly.
  if (p1_coin_collected || p2_coin_collected) {
    state.coin_pos = get_random_spawn_pos(state.rng);
  }
  ++state.age;
}

} // namespace jnb
