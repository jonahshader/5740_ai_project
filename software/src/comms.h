#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <variant>

#include "core.h"
#include "parse_map.h"
#include "optimizers/simple.h"

namespace jnb {

struct GAStatus {
  uint16_t current_gen{0};
  int16_t reference_fitness{0};
};

enum PSPLState { WAIT_FOR_UART_CONN, IDLE, TRAINING, PLAYING };

using get_uart_blocking_fun = std::function<std::uint8_t(void)>;
using get_uart_non_blocking_fun = std::function<std::optional<std::uint8_t>(void)>;
using set_uart_fun = std::function<void(std::uint8_t)>;

template <typename... Ts> struct Overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts> Overload(Ts...) -> Overload<Ts...>;

using msg_obj = std::variant<GameState, GAStatus, std::uint8_t, std::vector<std::uint8_t>>;

constexpr size_t MAX_POPULATION_SIZE = 128;
constexpr size_t MAP_MAX_SIZE_BITS = 4;
constexpr size_t MAP_MAX_SIZE_TILES = 1 << MAP_MAX_SIZE_BITS;

constexpr size_t MAP_MAX_SPAWNS = MAP_MAX_SIZE_TILES * MAP_MAX_SIZE_TILES / 2;

// comms_rx.vhd
constexpr uint8_t TILEMAP_MSG = 1;
constexpr uint8_t GA_CONFIG_MSG = 2;
constexpr uint8_t TRAINING_STOP_MSG = 3;
constexpr uint8_t PLAYER_INPUT_MSG = 4;
constexpr uint8_t TEST_MSG = 5;
constexpr uint8_t INFERENCE_GO_MSG = 6;
constexpr uint8_t TRAINING_GO_MSG = 0x0B;
constexpr uint8_t TRAINING_RESUME_MSG = 0x08;
constexpr uint8_t INFERENCE_STOP_MSG = 0x07;
constexpr uint8_t PLAY_AGAINST_NN_TRUE = 0x09;
constexpr uint8_t PLAY_AGAINST_NN_FALSE = 0x0a;
constexpr uint8_t BRAM_DUMP_MSG = 0x0C;

// comms_tx.vhd
constexpr uint8_t GA_STATUS_MSG = 1;
constexpr uint8_t GAMESTATE_MSG = 2;
constexpr uint8_t TEST_RESPONSE_MSG = 0x68;
constexpr uint8_t NE_IS_IDLE = 0x03;
constexpr uint8_t NE_IS_TRAINING = 0x04;
constexpr uint8_t NE_IS_PLAYING = 0x05;
constexpr uint8_t SEND_BRAM_MSG = 0x06;

void send(const GAConfig &ga, const EvalConfig &eval, const set_uart_fun &send_fun);
void send(const TileMap &map, const set_uart_fun &send_fun);
void send(const PlayerInput &player_input, const set_uart_fun &send_fun);

std::optional<msg_obj> receive(const get_uart_blocking_fun &get_fun_blocking,
                               const get_uart_non_blocking_fun &get_fun);

} // namespace jnb
