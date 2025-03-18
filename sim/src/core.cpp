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

  // determine if the player is grounded
  bool grounded = false;
  const int y_tile = p.y.to_integer() / CELL_SIZE;
  if (Fixed3(static_cast<int16_t>(y_tile * CELL_SIZE)) == p.y) {
    // we are on the bottom of a tile,
    // so check if we are on something stand-able...
    const int x_tile = p.x.to_integer() / CELL_SIZE;
    // check both the left and right side of the player
    if (can_stand_on(read_base_map(x_tile, y_tile - 1)) ||
        can_stand_on(read_base_map(x_tile + PLAYER_WIDTH - 1, y_tile - 1))) {
      grounded = true;
    }
  }

  if (grounded && input.jump) {
    p.y_vel = JUMP_VEL;
  }
  // accelerate x_vel based on input
  if (input.left && !input.right) {
    // accel left
    p.x_vel -= MOVE_ACCEL;
    // limit
    if (p.x_vel < -MOVE_MAX_VEL) {
      p.x_vel = -MOVE_MAX_VEL;
    }
  } else if (input.right && !input.left) {
    // accel right
    p.x_vel += MOVE_ACCEL;
    // limit
    if (p.x_vel > MOVE_MAX_VEL) {
      p.x_vel = MOVE_MAX_VEL;
    }
  } else {
    // decelerate towards zero
    if (p.x_vel > FIXED3_ZERO) {
      // check if we have room to do the full speed reduction
      if (p.x_vel >= MOVE_MAX_VEL) {
        p.x_vel -= MOVE_MAX_VEL;
      } else {
        // we are too slow to do the full speed reduction
        p.x_vel = FIXED3_ZERO;
      }
    } else if (p.x_vel < FIXED3_ZERO) {
      // check if we have room to do the full speed reduction
      if (p.x_vel <= -MOVE_MAX_VEL) {
        p.x_vel += MOVE_MAX_VEL;
      } else {
        // we are too slow to do the full speed reduction
        p.x_vel = FIXED3_ZERO;
      }
    }
  }
}

void update(GameState &state, const PlayerInput &in1, const PlayerInput &in2) {
  // get grounded status
  // (can be calculated in parallel on FPGA)
  bool p1_grounded = is_grounded(state.p1);
  bool p2_grounded = is_grounded(state.p2);

  ++state.age;
}

} // namespace jnb
