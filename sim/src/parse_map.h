// partially written by claude: https://claude.ai/share/cb896794-5297-453d-bfe6-45754e640540
// this is the simplest way of loading map files dynamically, as the libraries all
// attempt to be feature-complete, and depend on xml libraries. we only care about
// the raw map data, so this is fine.

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include <algorithm>

namespace jnb {
struct TilePos {
  uint8_t x;
  uint8_t y;
};

enum Tile {
  NOTHING = 0,
  GROUND = 1,
  AIR = 2,
  SPRING = 3,
  WATER_BODY = 4,
  WATER_TOP = 5,
  ICE = 6,
  COIN = 7
};

constexpr bool is_solid(Tile tile) {
  return tile == GROUND || tile == SPRING || tile == ICE;
}

constexpr bool is_water(Tile tile) {
  return tile == WATER_BODY || tile == WATER_TOP;
}

struct TileMap {
  int width;
  int height;
  std::vector<std::vector<std::uint8_t>> tiles;
  std::vector<TilePos> spawns;

  // Constructor that initializes an empty map
  TileMap() : width(0), height(0) {}

  // functions for reading the map in y-up ordering
  Tile read_map(int x, int y) const {
    if (x < 0 || y < 0 || x >= width)
      return Tile::GROUND;
    if (y >= height)
      return Tile::AIR;
    return static_cast<Tile>(tiles[height - 1 - y][x]);
  }

  Tile read_base_map(const TilePos &pos) const {
    return read_map(pos.x, pos.y);
  }

  // Parse map data from a file
  bool load_from_file(const std::string &filename) {
    std::cout << "Loading " << filename << std::endl;
    std::ifstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Failed to open file: " << filename << std::endl;
      return false;
    }

    std::string line;
    bool in_data = false;

    // Read map dimensions
    while (std::getline(file, line)) {
      // Extract width and height from the map tag
      size_t width_pos = line.find("width=\"");
      size_t height_pos = line.find("height=\"");

      if (width_pos != std::string::npos && height_pos != std::string::npos) {
        width_pos += 7; // Skip "width=""
        size_t width_end = line.find("\"", width_pos);
        width = std::stoi(line.substr(width_pos, width_end - width_pos));

        height_pos += 8; // Skip "height=""
        size_t height_end = line.find("\"", height_pos);
        height = std::stoi(line.substr(height_pos, height_end - height_pos));

        // Initialize the tiles vector with the correct dimensions
        tiles.resize(height, std::vector<std::uint8_t>(width, 0));
        continue;
      }

      // Look for data section start
      if (line.find("<data encoding=\"csv\">") != std::string::npos) {
        in_data = true;
        continue;
      }

      // Check for data section end
      if (in_data) {
        if (line.find("</data>") != std::string::npos) {
          break; // We're done parsing
        }

        // Parse the CSV data
        std::istringstream ss(line);
        std::string token;
        int row = 0, col = 0;

        // Find which row we're on by counting filled rows
        for (const auto &r : tiles) {
          if (!r.empty() && r[0] != 0) {
            row++;
          }
        }

        // Parse comma-separated values
        while (std::getline(ss, token, ',')) {
          // Remove trailing commas, newlines, or whitespace
          token.erase(std::remove_if(token.begin(), token.end(),
                                     [](unsigned char c) {
                                       return c == ',' || c == '\n' || c == '\r' || c == ' ';
                                     }),
                      token.end());

          if (!token.empty()) {
            std::uint8_t tile_id = static_cast<std::uint8_t>(std::stoi(token));
            if (col < width) {
              tiles[row][col] = tile_id;
              col++;
            }
          }
        }
      }
    }

    // we want to spawn coins/players in the air, but above non-air (so that they are accessible).
    // for FPGA implementation, this can be cached.

    // populate the list of potential spawn locations
    spawns.clear();
    for (int y = 0; y < height - 1; ++y) {
      for (int x = 0; x < width; ++x) {
        if (is_solid(static_cast<Tile>(read_map(x, y))) && read_map(x, y + 1) == AIR) {
          spawns.emplace_back(TilePos(x, static_cast<uint8_t>(y + 1)));
        }
      }
    }

    return true;
  }

  // Print the map for debugging
  void print_map() const {
    std::cout << "Map dimensions: " << width << "x" << height << std::endl;
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        std::cout << static_cast<int>(tiles[y][x]) << " ";
      }
      std::cout << std::endl;
    }
  }
};

} // namespace jnb
