#include "jnb.h"

#include <cmath>
#include <functional>
#include <vector>

namespace jnb {

// read from the base map with y-up indexing,
// out-of-bounds down, left, right returns GROUND.
// out-of-bounds up returns AIR.

TilePos get_random_spawn_pos(std::mt19937 &rng, const TileMap &map) {
  std::uniform_int_distribution<int> dist(0, map.spawns.size() - 1);
  auto coin_pos_index = dist(rng);
  return map.spawns[coin_pos_index];
}

GameState init(const std::string &map_filename, uint64_t seed) {
  GameState state{};
  // load map
  state.map.load_from_file(map_filename);
  // init the rest of state
  reinit(state, seed);
  return state;
}

void reinit(GameState &state, uint64_t seed) {
  // clear some things
  state.p1 = {};
  state.p2 = {};
  state.age = 0;

  // create rng from seed
  state.rng = std::mt19937(seed);
  // pick random position for coin
  state.coin_pos = get_random_spawn_pos(state.rng, state.map);
  // pick random positions for p1, p2
  std::uniform_int_distribution<int> spawn_index_dist(0, state.map.spawns.size() - 1);
  auto spawn_index = spawn_index_dist(state.rng);
  auto spawn = state.map.spawns[spawn_index];
  state.p1.x = F4(static_cast<int16_t>(spawn.x * CELL_SIZE));
  state.p1.y = F4(static_cast<int16_t>(spawn.y * CELL_SIZE));
  auto spawn_index_2 = spawn_index_dist(state.rng);
  // ensure p2 doesn't spawn on p1
  // TODO: extend to coin logic? or is this needlessly complicated for FPGA impl?
  if (spawn_index_2 == spawn_index) {
    spawn_index_2 = (spawn_index + 1) % state.map.spawns.size();
  }
  spawn = state.map.spawns[spawn_index_2];
  state.p2.x = F4(static_cast<int16_t>(spawn.x * CELL_SIZE));
  state.p2.y = F4(static_cast<int16_t>(spawn.y * CELL_SIZE));
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

// building the phases as lambdas within a function so that I can create and capture
// some variables that otherwise would have to be re-computed, or passed, to each phase.
// TODO: on fpga we might be able to mash these phases together, because we can update
// velocity without it taking effect immediately (i.e., we only see that register change
// on the next cycle).
std::vector<std::function<void(Player &, const Player &, const PlayerInput &, bool &)>>
make_player_phases(const Player &_p, GameState &state) {
  // these values will be computed combinatorially on FPGA

  // x_low and y_low is the bottom left of the player, in pixels
  const int x_low = _p.x.to_integer_floor();
  const int y_low = _p.y.to_integer_floor();

  const int x_tile_left = get_tile_id(x_low); // tile x coord containing left side of player
  const int x_tile_right =
      get_tile_id(x_low + PLAYER_WIDTH - 1);  // tile y coord containing right side of player
  const int y_tile_down = get_tile_id(y_low); // tile y coord containing bottom of player
  const int y_tile_up =
      get_tile_id(y_low + PLAYER_HEIGHT - 1); // tile y coord containing top of player

  // have the tile coordinates, now we can retrieve the tiles we care about
  const Tile left_tile = state.map.read_map(x_tile_left, y_tile_down); // tile our left foot is in
  const Tile right_tile =
      state.map.read_map(x_tile_right, y_tile_down); // tile our right foot is in
  const Tile down_left_tile =
      state.map.read_map(x_tile_left, y_tile_down - 1); // tile below left_tile
  const Tile down_right_tile =
      state.map.read_map(x_tile_right, y_tile_down - 1); // tile below right_tile

  auto phase1 = [=, &state](Player &p, const Player &other, const PlayerInput &input,
                            bool &coin_collected) {
    // early return if dead
    if (p.dead_timeout > 0)
      return;
    // determine if the player is grounded
    bool grounded = false;
    if (F4(static_cast<int16_t>(y_tile_down * CELL_SIZE)) == p.y) {
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
    const F4 gravity = in_water ? GRAVITY_WATER : GRAVITY;
    const F4 move_accel = on_ice ? MOVE_ACCEL_ICE : (in_water ? MOVE_ACCEL_WATER : MOVE_ACCEL);

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
      // limit y_vel
      if (p.y_vel < FALL_MAX_VEL) {
        p.y_vel = FALL_MAX_VEL;
      }
    }

    // accelerate x_vel based on input
    if (input.left && !input.right) {
      // accel left
      p.x_vel -= move_accel;
    } else if (input.right && !input.left) {
      // accel right
      p.x_vel += move_accel;
    } else {
      // decelerate towards zero if grounded and not on ice
      if (grounded && !on_ice) {
        if (p.x_vel > F4_ZERO) {
          // check if we have room to do the full speed reduction
          if (p.x_vel >= move_accel) {
            p.x_vel -= move_accel;
          } else {
            // we are too slow to do the full speed reduction
            p.x_vel = F4_ZERO;
          }
        } else if (p.x_vel < F4_ZERO) {
          // check if we have room to do the full speed reduction
          if (p.x_vel <= -move_accel) {
            p.x_vel += move_accel;
          } else {
            // we are too slow to do the full speed reduction
            p.x_vel = F4_ZERO;
          }
        }
      }
    }

    // limit x velocity
    if (p.x_vel < -MOVE_MAX_VEL) {
      p.x_vel = -MOVE_MAX_VEL;
    }
    if (p.x_vel > MOVE_MAX_VEL) {
      p.x_vel = MOVE_MAX_VEL;
    }

    // accelerate based on collision with other player
    // just push away in the horizontal axis during collision
    // HACK: because these phases run sequentially on CPU,
    // this player might die and be skipped by the next phase, which
    // is asymmetric
    if (other.dead_timeout == 0) { // only run if opponent is alive
      if ((p.y - other.y).abs() < F4(static_cast<int16_t>(PLAYER_HEIGHT))) {
        if ((p.x - other.x).abs() <= F4(static_cast<int16_t>(PLAYER_WIDTH))) {
          bool accel = true;
          // if other player is significantly above this one, die
          if (other.y >= p.y + F4(static_cast<int16_t>(PLAYER_KILL_HEIGHT))) {
            p.queue_dead = true;
          }
          // if the opposite is true, gain a point
          else if (p.y >= other.y + F4(static_cast<int16_t>(PLAYER_KILL_HEIGHT))) {
            p.score += POINTS_PER_KILL;
            accel = false;
          }
          if (accel) {
            if (p.x > other.x) {
              p.x_vel += (other.x - p.x + F4(static_cast<int16_t>(PLAYER_WIDTH)));
            } else if (p.x < other.x) {
              p.x_vel += (other.x - p.x - F4(static_cast<int16_t>(PLAYER_WIDTH)));
            }
          }
        }
      }
    }
  };

  auto phase2 = [=, &state](Player &p, const Player &other, const PlayerInput &input,
                            bool &coin_collected) {
    // early return if dead
    if (p.dead_timeout > 1) {
      p.dead_timeout--;
      return;
    } else if (p.dead_timeout == 1) {
      p.dead_timeout--;
      // respawn
      auto tile_pos = get_random_spawn_pos(state.rng, state.map);
      p.x = F4(static_cast<int16_t>(tile_pos.x * CELL_SIZE));
      p.y = F4(static_cast<int16_t>(tile_pos.y * CELL_SIZE));
      p.x_vel = F4_ZERO;
      p.y_vel = F4_ZERO;
    }

    // integrate velocity
    p.x += p.x_vel;
    p.y += p.y_vel;

    // handle collisions against solid tiles
    const int xn_low = p.x.to_integer_floor();
    const int yn_low = p.y.to_integer_floor();

    const int xn_tile_left = get_tile_id(xn_low);
    const int xn_tile_right = get_tile_id(xn_low + PLAYER_WIDTH - 1);
    const int yn_tile_down = get_tile_id(yn_low);
    const int yn_tile_up = get_tile_id(yn_low + PLAYER_HEIGHT - 1);

    // handle left right collisions
    if (p.x_vel < F4_ZERO) {
      // going left. check left side
      const Tile left_1 = state.map.read_map(xn_tile_left, y_tile_down);
      const Tile left_2 = state.map.read_map(xn_tile_left, y_tile_up);
      if (is_solid(left_1) || is_solid(left_2)) {
        // make flush with wall
        p.x = F4(static_cast<int16_t>((xn_tile_left + 1) * CELL_SIZE));
        // cancel velocity
        p.x_vel = F4_ZERO;
      }
    } else if (p.x_vel > F4_ZERO) {
      // going right. check right side
      const Tile right_1 = state.map.read_map(xn_tile_right, y_tile_down);
      const Tile right_2 = state.map.read_map(xn_tile_right, y_tile_up);
      if (is_solid(right_1) || is_solid(right_2)) {
        // make flush with wall
        p.x = F4(static_cast<int16_t>(xn_tile_right * CELL_SIZE - PLAYER_WIDTH));
        // cancel velocity
        p.x_vel = F4_ZERO;
      }
    }
    if (p.y_vel < F4_ZERO) {
      // going down. check bottom
      const Tile down_1 = state.map.read_map(x_tile_left, yn_tile_down);
      const Tile down_2 = state.map.read_map(x_tile_right, yn_tile_down);
      if (is_solid(down_1) || is_solid(down_2)) {
        // make flush with floor
        p.y = F4(static_cast<int16_t>((yn_tile_down + 1) * CELL_SIZE));
        // cancel velocity
        p.y_vel = F4_ZERO;
      }
    } else if (p.y_vel > F4_ZERO) {
      // going up. check top
      const Tile top_1 = state.map.read_map(x_tile_left, yn_tile_up);
      const Tile top_2 = state.map.read_map(x_tile_right, yn_tile_up);
      if (is_solid(top_1) || is_solid(top_2)) {
        p.y = F4(static_cast<int16_t>(yn_tile_up * CELL_SIZE - PLAYER_HEIGHT));
        // cancel velocity
        p.y_vel = F4_ZERO;
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
    coin_collected = x_tile_center == state.coin_pos.x && y_tile_center == state.coin_pos.y;
    if (coin_collected) {
      // get a point for collecting coin
      p.score += POINTS_PER_COIN;
    }

    // if death was queued, die
    if (p.queue_dead) {
      p.queue_dead = false;
      p.dead_timeout = DEAD_TIMEOUT;
    }
  };

  return {phase1, phase2};
}

void update(GameState &state, const PlayerInput &in1, const PlayerInput &in2) {
  // updating can happen in parallel in FPGA
  auto p1_phases = make_player_phases(state.p1, state);
  auto p2_phases = make_player_phases(state.p2, state);

  // run each phase for each player
  bool p1_coin_collected = false;
  bool p2_coin_collected = false;
  for (int i = 0; i < p1_phases.size(); ++i) {
    // these can be concurrent on FPGA
    p1_phases[i](state.p1, state.p2, in1, p1_coin_collected);
    p2_phases[i](state.p2, state.p1, in2, p2_coin_collected);
  }

  // some other ideas: maybe the player could place a temporary ground tile
  // beneath them, at the expense of one point (the coin reward would have to
  // be much higher, so that its still worth placing ground tiles if it means
  // increased likelyhood of getting the coin)

  // if the coin was collected,
  // pick a new location from the valid coin spawn locations randomly.
  // this must happen on a new cycle, so the number of cycles on fpga is phases.size() + 1
  if (p1_coin_collected || p2_coin_collected) {
    state.coin_pos = get_random_spawn_pos(state.rng, state.map);
  }
  ++state.age;
}

void observe_state_simple(const GameState &state, std::vector<F4> &observation,
                          bool p1_perspective) {
  observation.resize(SIMPLE_INPUT_COUNT);
  size_t index = 0;
  // coin pos
  observation[index++] = F4(static_cast<int16_t>(state.coin_pos.x * CELL_SIZE));
  observation[index++] = F4(static_cast<int16_t>(state.coin_pos.y * CELL_SIZE));
  // determine player state order based on who's observing (p1_perspective)
  const Player &first = p1_perspective ? state.p1 : state.p2;
  const Player &second = p1_perspective ? state.p2 : state.p1;
  // first player pos
  observation[index++] = first.x;
  observation[index++] = first.y;
  // first player vel
  observation[index++] = first.x_vel;
  observation[index++] = first.y_vel;
  // players dead
  observation[index++] = first.dead_timeout > 0 ? F4(1.0f) : F4(-1.0f);
  observation[index++] = second.dead_timeout > 0 ? F4(1.0f) : F4(-1.0f);
  // second player pos
  observation[index++] = second.x;
  observation[index++] = second.y;
  // second player vel
  observation[index++] = second.x_vel;
  observation[index++] = second.y_vel;
}

void observe_state_simple(const GameState &state, std::vector<float> &observation,
                          bool p1_perspective) {
  const float x_norm = 1.0f / (state.map.width * CELL_SIZE);
  const float y_norm = 1.0f / (state.map.height * CELL_SIZE);
  const float x_vel_norm = 1.0f / (MOVE_MAX_VEL.to_float());
  const float y_vel_norm = 1.0f / (-FALL_MAX_VEL.to_float());

  observation.resize(SIMPLE_INPUT_COUNT);
  size_t index = 0;
  // coin pos
  observation[index++] = F4(static_cast<int16_t>(state.coin_pos.x * CELL_SIZE)).to_float() * x_norm;
  observation[index++] = F4(static_cast<int16_t>(state.coin_pos.y * CELL_SIZE)).to_float() * y_norm;
  // determine player state order based on who's observing (p1_perspective)
  const Player &first = p1_perspective ? state.p1 : state.p2;
  const Player &second = p1_perspective ? state.p2 : state.p1;
  // first player pos
  observation[index++] = first.x.to_float() * x_norm;
  observation[index++] = first.y.to_float() * y_norm;
  // first player vel
  observation[index++] = first.x_vel.to_float() * x_vel_norm;
  observation[index++] = first.y_vel.to_float() * y_vel_norm;
  // players dead
  observation[index++] = first.dead_timeout > 0 ? 1000.0f : 0.0f;
  observation[index++] = second.dead_timeout > 0 ? 1000.0f : 0.0f;
  // second player pos
  observation[index++] = second.x.to_float() * x_norm;
  observation[index++] = second.y.to_float() * y_norm;
  // second player vel
  observation[index++] = second.x_vel.to_float() * x_vel_norm;
  observation[index++] = second.y_vel.to_float() * y_vel_norm;
}

int get_fitness(const GameState &state, bool p1_perspective) {
  if (p1_perspective) {
    return state.p1.score - state.p2.score;
  } else {
    return state.p2.score - state.p1.score;
  }
}

} // namespace jnb
