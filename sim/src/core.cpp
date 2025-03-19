#include "core.h"

namespace jnb {

// read from the base map with y-up indexing,
// out-of-bounds down, left, right returns GROUND.
// out-of-bounds up returns AIR.
constexpr Tile read_base_map(int x, int y) {
  if (x < 0 || y < 0 || x >= WIDTH_CELLS)
    return Tile::GROUND;
  if (y >= HEIGHT_CELLS)
    return Tile::AIR;
  return static_cast<Tile>(base_map[HEIGHT_CELLS - 1 - y][x]);
}

void update_player(Player &p, const PlayerInput &input, const TilePos &coin_pos, bool &coin_collected) {
  constexpr Fixed3 F3_ZERO = Fixed3::from_raw(0);

  const int x_low = p.x.to_integer_floor();
  const int x_high = p.x.to_integer_ceil();
  const int y_low = p.y.to_integer_floor();
  const int y_high = p.y.to_integer_ceil();

  const int x_tile_left = x_low / CELL_SIZE;
  const int x_tile_right = (x_high + PLAYER_WIDTH - 1) / CELL_SIZE;
  const int y_tile_down = y_low / CELL_SIZE;
  const int y_tile_up = (y_high + PLAYER_HEIGHT - 1) / CELL_SIZE;

  // get the tiles the player is standing on
  const Tile down_left_tile = read_base_map(x_tile_left, y_tile_down - 1);
  const Tile down_right_tile = read_base_map(x_tile_right, y_tile_down - 1);
  const Tile left_tile = read_base_map(x_tile_left, y_tile_down);
  const Tile right_tile = read_base_map(x_tile_right, y_tile_down);

  // determine if the player is grounded
  bool grounded = false;
  if (Fixed3(static_cast<int16_t>(y_tile_down * CELL_SIZE)) == p.y) {
    // we are on the bottom of a tile,
    // so check if we are on something stand-able...
    if (is_solid(down_left_tile) || is_solid(down_right_tile)) {
      grounded = true;
    }
  }

  // determine if in water
  const bool in_water = is_water(left_tile) || is_water(right_tile);
  // determine if on ice
  const bool on_ice = left_tile == Tile::ICE || right_tile == Tile::ICE;
  // determine acceleration based on context
  const Fixed3 gravity = in_water ? GRAVITY_WATER : GRAVITY;
  const Fixed3 move_accel = on_ice ? MOVE_ACCEL_ICE : (in_water ? MOVE_ACCEL_WATER : MOVE_ACCEL);

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
    // decelerate towards zero
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

  // integrate the velocities
  p.x += p.x_vel;
  p.y += p.y_vel;

  // handle collisions against solid tiles
  const int xn_low = p.x.to_integer_floor();
  const int xn_high = p.x.to_integer_ceil();
  const int yn_low = p.y.to_integer_floor();
  const int yn_high = p.y.to_integer_ceil();

  const int xn_tile_left = xn_low / CELL_SIZE;
  const int xn_tile_right = (xn_high + PLAYER_WIDTH - 1) / CELL_SIZE;
  const int yn_tile_down = yn_low / CELL_SIZE;
  const int yn_tile_up = (yn_high + PLAYER_HEIGHT - 1) / CELL_SIZE;

  // handle left right collisions
  if (p.x_vel < F3_ZERO) {
    // going left. check left side
    const Tile left_1 = read_base_map(xn_tile_left, y_tile_down);
    const Tile left_2 = read_base_map(xn_tile_left, y_tile_up);
    if (is_solid(left_1) || is_solid(left_2)) {
      // make flush with wall
      p.x = Fixed3(static_cast<int16_t>((xn_tile_left + 1) * CELL_SIZE));
      // cancel velocity
      p.x_vel = F3_ZERO;
    }
  } else if (p.x_vel > F3_ZERO) {
    // going right. check right side
    const Tile right_1 = read_base_map(xn_tile_right, y_tile_down);
    const Tile right_2 = read_base_map(xn_tile_right, y_tile_up);
    if (is_solid(right_1) || is_solid(right_2)) {
      // make flush with wall
      p.x = Fixed3(static_cast<int16_t>((xn_tile_right - 1) * CELL_SIZE));
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
      p.y = Fixed3(static_cast<int16_t>((yn_tile_down + 1) * CELL_SIZE));
      // cancel velocity
      p.y_vel = F3_ZERO;
    }
  } else if (p.y_vel > F3_ZERO) {
    // going up. check top
    const Tile top_1 = read_base_map(x_tile_left, yn_tile_up);
    const Tile top_2 = read_base_map(x_tile_right, yn_tile_up);
    if (is_solid(top_1) || is_solid(top_2)) {
      p.y = Fixed3(static_cast<int16_t>((yn_tile_up - 1) * CELL_SIZE));
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
  const int x_tile_center = x_center / CELL_SIZE;
  const int y_tile_center = y_center / CELL_SIZE;
  coin_collected = x_tile_center == coin_pos.x && y_tile_center == coin_pos.y;
  if (coin_collected) {
    // get a point for collecting coin
    ++p.score;
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
    std::uniform_int_distribution<int> coin_pos_dist(0, COIN_SPAWN_COUNT);
    auto coin_pos_index = coin_pos_dist(state.rng);
    state.coin_pos = COIN_SPAWN_POSITIONS[coin_pos_index];
  }
  ++state.age;
}

} // namespace jnb
