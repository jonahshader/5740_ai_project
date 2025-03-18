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

void update_player(Player &p, const PlayerInput &input) {
  constexpr Fixed3 FIXED3_ZERO = Fixed3::from_raw(0);

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
    if (p.x_vel > FIXED3_ZERO) {
      // check if we have room to do the full speed reduction
      if (p.x_vel >= move_accel) {
        p.x_vel -= move_accel;
      } else {
        // we are too slow to do the full speed reduction
        p.x_vel = FIXED3_ZERO;
      }
    } else if (p.x_vel < FIXED3_ZERO) {
      // check if we have room to do the full speed reduction
      if (p.x_vel <= -move_accel) {
        p.x_vel += move_accel;
      } else {
        // we are too slow to do the full speed reduction
        p.x_vel = FIXED3_ZERO;
      }
    }
  }

  // integrate the velocities
  p.x += p.x_vel;
  p.y += p.y_vel;

  // TODO: handle collisions against solid tiles

}

void update(GameState &state, const PlayerInput &in1, const PlayerInput &in2) {
  ++state.age;
}

} // namespace jnb
