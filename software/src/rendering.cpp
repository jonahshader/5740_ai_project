#include "rendering.h"

namespace rendering {

uint32_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | r;
  return rgba;
}

void draw_tile(std::vector<uint32_t> &pixels, std::pair<int, int> pixels_res,
               const std::vector<uint8_t> &spritesheet, int x_tile, int y_tile, int tile_size,
               int t_id) {
  for (int y = 0; y < tile_size; ++y) {
    for (int x = 0; x < tile_size; ++x) {
      const uint8_t *rgb = &spritesheet[4 * (x + (y + t_id * tile_size) * tile_size)];
      size_t index =
          (x + x_tile * tile_size) + (y + y_tile * tile_size) * pixels_res.first;
      pixels[index] = make_color(rgb[0], rgb[1], rgb[2], 255);
    }
  }
}

void draw_map(std::vector<uint32_t> &pixels, std::pair<int, int> pixels_res,
              const std::vector<uint8_t> &spritesheet,
              const std::vector<std::vector<std::uint8_t>> &tiles, int tile_size) {
  for (int y_tile = 0; y_tile < tiles.size(); ++y_tile) {
    for (int x_tile = 0; x_tile < tiles[0].size(); ++x_tile) {
      const uint8_t t_id = tiles[y_tile][x_tile] - 1;
      draw_tile(pixels, pixels_res, spritesheet, x_tile, y_tile, tile_size, t_id);
    }
  }
}

void draw_rect(std::vector<uint32_t> &pixels, std::pair<int, int> pixels_res, int x, int y, int w,
               int h, uint32_t color) {
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; ++j) {
      int px = x + j;
      int py = y + i;
      // continue if out of bounds
      if (px < 0 || px >= pixels_res.first || py < 0 || py >= pixels_res.second) {
        continue;
      }
      size_t index = px + py * pixels_res.first;
      pixels[index] = color;
    }
  }
}

} // namespace rendering
