#pragma once

#include <cstdint>
#include <functional>

#include "core.h"
#include "parse_map.h"
#include "optimizers/simple.h"

namespace jnb {

using get_uart_fun = std::function<std::uint8_t(void)>;
using set_uart_fun = std::function<void(std::uint8_t)>;

constexpr size_t MAX_POPULATION_SIZE = 128;
constexpr size_t MAP_MAX_SIZE_BITS = 4;
constexpr size_t MAP_MAX_SIZE_TILES = 1 << MAP_MAX_SIZE_BITS;

constexpr size_t MAP_MAX_SPAWNS = MAP_MAX_SIZE_TILES * MAP_MAX_SIZE_TILES / 2;

constexpr uint8_t TILEMAP_MSG = 1;
constexpr uint8_t GA_CONFIG_MSG = 2;
constexpr uint8_t TRAINING_STOP_MSG = 3;
constexpr uint8_t PLAYER_INPUT_MSG = 4;
constexpr uint8_t TEST_MSG = 5;
constexpr uint8_t INFERENCE_GO_MSG = 6;

void send(const GAConfig &ga, const EvalConfig &eval, const set_uart_fun &send_fun);
void send(const TileMap &map, const set_uart_fun &send_fun);
void send(const PlayerInput &player_input, const set_uart_fun &send_fun);

} // namespace jnb
