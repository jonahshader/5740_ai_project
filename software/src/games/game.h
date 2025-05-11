#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <utility>

template <typename ObsType> class Game {
public:
  virtual ~Game() = default;

  // Initialize or reset the game state
  virtual void init(uint64_t seed) = 0;

  // Update the game with actions from all players
  virtual void update(const std::vector<std::vector<float>> &actions) = 0;

  // Get the number of action dimensions for each player
  virtual size_t get_action_count() = 0;

  // Get the number of players
  virtual size_t get_player_count() = 0;

  // Get fitness for all players (filled into the provided vector)
  virtual void get_fitness(std::vector<int32_t> &fitness) = 0;

  // Check if game is finished
  virtual bool is_done() = 0;

  // Fill the inputs vector with observations for each player
  virtual void observe(std::vector<ObsType> &inputs) = 0;

  // Create an initial observation vector, which is reused in the gameplay loop
  virtual std::vector<ObsType> build_observation() = 0;

  // Get the game name (for logging/identification)
  virtual std::string get_name() = 0;

  // Render the game state to a pixel buffer
  virtual void render(std::vector<uint32_t> &pixels) = 0;

  // Get the rendering resolution (width, height)
  virtual std::pair<int, int> get_resolution() = 0;

  // Clone the game state (deep copy)
  virtual std::unique_ptr<Game<ObsType>> clone() const = 0;
};
