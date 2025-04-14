#pragma once

#include <cstdint>
#include <functional>

#include "core.h"
#include "parse_map.h"

namespace jnb {

using get_uart_fun = std::function<std::uint8_t(void)>;
using set_uart_fun = std::function<void(std::uint8_t)>;

} // namespace jnb