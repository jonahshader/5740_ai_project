#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace rendering {

uint32_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void draw_tile(std::vector<uint32_t> &pixels, std::pair<int, int> pixels_res,
               const std::vector<uint8_t> &spritesheet, int x_tile, int y_tile, int tile_size,
               int t_id);

void draw_map(std::vector<uint32_t> &pixels, std::pair<int, int> pixels_res,
              const std::vector<uint8_t> &spritesheet,
              const std::vector<std::vector<std::uint8_t>> &tiles, int tile_size);

void draw_rect(std::vector<uint32_t> &pixels, std::pair<int, int> pixels_res, int x, int y, int w, int h,
                uint32_t color);

} // namespace rendering
