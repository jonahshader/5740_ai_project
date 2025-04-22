#include "comms.h"

namespace jnb {

void send(const GAConfig &ga, const EvalConfig &eval, const set_uart_fun &send_fun) {
  // send message indicating we are about to transfer the ga config
  send_fun(GA_CONFIG_MSG);

  // TR_GA_MUTATION_RATES

  // send mutation_rates
  // in hardware, a mutation rate is a byte where 0 -> 0.0, 255 -> 1.0.
  // the fixed_point interpretation is a probability of mutating.
  // the hardware expects an array of mutation rates, which allows us to bake in
  // tapering, or any other curves (TODO: try quadratic tapering?)
  for (int i = 0; i < MAX_POPULATION_SIZE; ++i) {
    float mr = ga.mutation_rate;
    if (ga.taper_mutation_rate) {
      mr *= (i / (ga.population_size - 1.0f));
    }
    if (i >= ga.population_size) {
      mr = 0;
    }
    // send it as a byte
    send_fun(static_cast<uint8_t>(mr));
  }

  // TR_GA_MAX_GEN

  // send max generations, 2 bytes in hardware.
  // send upper byte first
  send_fun(static_cast<uint8_t>(ga.max_gen >> 8));
  send_fun(static_cast<uint8_t>(ga.max_gen));

  // TR_GA_RUN_UNTIL_STOP_CMD

  // just a boolean - run_until_stop
  send_fun(static_cast<uint8_t>(ga.run_until_stop));

  // TR_GA_TOURNAMENT_SIZE
  send_fun(static_cast<uint8_t>(ga.tournament_size));
  // TR_GA_POPULATION_SIZE_EXP
  // the log of population_size
  send_fun(static_cast<uint8_t>(round(log2(ga.population_size))));
  // TR_GA_MODEL_HISTORY_SIZE
  send_fun(static_cast<uint8_t>(ga.model_history_size));
  // TR_GA_MODEL_HISTORY_INTERVAL
  send_fun(static_cast<uint8_t>(ga.model_history_interval));
  // TR_GA_SEED
  // seed is 4 bytes in hardware (8 in software)
  send_fun(static_cast<uint8_t>(ga.seed >> 24));
  send_fun(static_cast<uint8_t>(ga.seed >> 16));
  send_fun(static_cast<uint8_t>(ga.seed >> 8));
  send_fun(static_cast<uint8_t>(ga.seed));
  // TR_GA_REFERENCE_COUNT
  send_fun(static_cast<uint8_t>(ga.reference_count));
  // TR_GA_EVAL_INTERVAL
  send_fun(static_cast<uint8_t>(ga.eval_interval));
  // TR_GA_SEED_COUNT
  send_fun(static_cast<uint8_t>(eval.seed_count));
  // TR_GA_FRAME_LIMIT
  // send upper byte first
  send_fun(static_cast<uint8_t>(eval.frame_limit >> 8));
  send_fun(static_cast<uint8_t>(eval.frame_limit));
  // done
}

void send(const TileMap &map, const set_uart_fun &send_fun) {
  // send message indicating we are about to transfer the map
  send_fun(TILEMAP_MSG);

  // TR_MAP_S
  // iterate over the max size of the map, sending NOTHING for out-of-bound tiles
  for (int y = 0; y < MAP_MAX_SIZE_TILES; ++y) {
    for (int x = 0; x < MAP_MAX_SIZE_TILES; ++x) {
      if (x < map.width && y < map.height) {
        // send the tile
        send_fun(static_cast<uint8_t>(map.tiles[y][x]));
      } else {
        // send empty tile
        send_fun(static_cast<uint8_t>(Tile::NOTHING));
      }
    }
  }

  // TR_MAP_SPAWNS_S
  // map spawns is 1d, but there are two numbers per coord.
  for (int i = 0; i < MAP_MAX_SPAWNS; ++i) {
    if (i >= map.spawns.size()) {
      // send 0 0
      send_fun(0);
      send_fun(0);
    } else {
      // send the spawn
      send_fun(map.spawns[i].x);
      send_fun(map.spawns[i].y);
    }
  }

  // TR_MAP_NUM_SPAWN_S
  send_fun(static_cast<uint8_t>(map.spawns.size()));
  // TR_MAP_NUM_SPAWN_BITS_S
  // send the number of bits needed to represent the number of spawns.
  send_fun(static_cast<uint8_t>(ceil(log2(map.spawns.size()))));
  // TR_MAP_WIDTH
  send_fun(static_cast<uint8_t>(map.width));
  // TR_MAP_HEIGHT
  send_fun(static_cast<uint8_t>(map.height));
}

void send(const PlayerInput &player_input, const set_uart_fun &send_fun) {
  // send message indicating we are about to transfer the player input
  send_fun(PLAYER_INPUT_MSG);

  // TR_PLAYER_INPUT
  // bit 0 is left
  // bit 1 is right
  // bit 2 is jump
  uint8_t msg = (player_input.left ? 0x01 : 0) | (player_input.right ? 0x02 : 0) |
                (player_input.jump ? 0x04 : 0);
  send_fun(msg);
}

std::optional<msg_obj> receive(const get_uart_blocking_fun &get_fun_blocking,
                               const get_uart_non_blocking_fun &get_fun) {
  // first get a byte without blocking
  auto msg = get_fun();

  // if we got nothing, return nothing
  if (!msg) {
    std::cout << "Receive function got nothing, so returning nullopt." << std::endl;
    return std::nullopt;
  }

  // check the message type
  switch (msg.value()) {
    case GA_STATUS_MSG: {
      std::cout << "Got GA_STATUS_MSG, so next serial receieves will go to GAStatus" << std::endl;
      // variant is GAStatus
      GAStatus ret;
      // TR_CURRENT_GEN_1_S
      // 2 bytes
      std::cout << "Receiving current_gen upper" << std::endl;
      ret.current_gen = get_fun_blocking() << 8;
      // TR_CURRENT_GEN_2_S
      std::cout << "Receiving current_gen lower" << std::endl;
      ret.current_gen |= get_fun_blocking();
      // TR_REFERENCE_FITNESS_1_S
      // 2 bytes
      std::cout << "Receiving reference_fitness upper" << std::endl;
      ret.reference_fitness = get_fun_blocking() << 8;
      // TR_REFERENCE_FITNESS_2_S
      std::cout << "Receiving reference_fitness lower" << std::endl;
      ret.reference_fitness |= get_fun_blocking();
      // return the GAStatus
      std::cout << "Returning GAStatus" << std::endl;
      return ret;
    } break;
    case GAMESTATE_MSG: {
      // variant is GameState
      GameState ret;
      // TR_P1_X_1_S
      // 2 bytes
      uint16_t v;
      v = get_fun_blocking() << 8;
      // TR_P1_X_2_S
      v |= get_fun_blocking();
      ret.p1.x = F4(static_cast<int16_t>(v));
      // TR_P1_Y_1_S
      // 2 bytes
      v = get_fun_blocking() << 8;
      // TR_P1_Y_2_S
      v |= get_fun_blocking();
      ret.p1.y = F4(static_cast<int16_t>(v));
      // TR_P1_SCORE_1_S
      // 2 bytes
      v = get_fun_blocking() << 8;
      // TR_P1_SCORE_2_S
      v |= get_fun_blocking();
      ret.p1.score = static_cast<int16_t>(v);
      // TR_P1_DEAD_TIMEOUT_S
      // 1 byte
      ret.p1.dead_timeout = get_fun_blocking();
      // TR_P2_X_1_S
      // 2 bytes
      v = get_fun_blocking() << 8;
      // TR_P2_X_2_S
      v |= get_fun_blocking();
      ret.p2.x = F4(static_cast<int16_t>(v));
      // TR_P2_Y_1_S
      // 2 bytes
      v = get_fun_blocking() << 8;
      // TR_P2_Y_2_S
      v |= get_fun_blocking();
      ret.p2.y = F4(static_cast<int16_t>(v));
      // TR_P2_SCORE_1_S
      // 2 bytes
      v = get_fun_blocking() << 8;
      // TR_P2_SCORE_2_S
      v |= get_fun_blocking();
      ret.p2.score = static_cast<int16_t>(v);
      // TR_P2_DEAD_TIMEOUT_S
      // 1 byte
      ret.p2.dead_timeout = get_fun_blocking();
      // TR_COIN_X_S
      // 1 byte
      ret.coin_pos.x = get_fun_blocking();
      // TR_COIN_Y_S
      // 1 byte
      ret.coin_pos.y = get_fun_blocking();
      // TR_AGE_1_S
      // 2 bytes
      v = get_fun_blocking() << 8;
      // TR_AGE_2_S
      v |= get_fun_blocking();
      ret.age = v;
      // return the GameState
      return ret;
    } break;
    default:
      // must be a state transition. return it as-is
      return msg.value();
  }

  // if we get here, we didn't return anything
  return std::nullopt;
}

} // namespace jnb
