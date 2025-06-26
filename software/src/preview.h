#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>

#include "games/game.h"
#include "models/model.h"
#include "optimizers/ga.h"
#include "play.h"

// Use the type definition from ga.h
template <typename ObsType>
using UpdateModelCallback = ga::PreviewCallback<ObsType>;

template <typename ObsType>
using PreviewControl = std::function<void()>;

// Creates a preview system that continuously runs games with the latest models
// Returns: (update_callback, start_function, stop_function)
template <typename ObsType>
auto create_preview(std::shared_ptr<Game<ObsType>> base_game) {
  // Shared state captured by lambdas
  auto models_a = std::make_shared<std::vector<std::shared_ptr<model::Model<ObsType>>>>();
  auto models_b = std::make_shared<std::vector<std::shared_ptr<model::Model<ObsType>>>>();
  auto current_models =
      std::make_shared<std::shared_ptr<std::vector<std::shared_ptr<model::Model<ObsType>>>>>(
          models_a);
  auto models_mutex = std::make_shared<std::mutex>();
  auto should_stop = std::make_shared<std::atomic<bool>>(false);
  auto preview_thread = std::make_shared<std::unique_ptr<std::thread>>(nullptr);
  auto game_seed = std::make_shared<std::uint64_t>(123);

  // Update callback - stores all available models
  auto update_callback =
      [=](std::shared_ptr<std::vector<std::shared_ptr<model::Model<ObsType>>>> new_models) {
        std::lock_guard<std::mutex> lock(*models_mutex);

        // Swap to the unused buffer and copy all available models
        if (*current_models == models_a) {
          *models_b = *new_models;
          *current_models = models_b;
        } else {
          *models_a = *new_models;
          *current_models = models_a;
        }
      };

  // Start function - begins the preview loop in a background thread
  auto start = [=]() {
    if (*preview_thread && (*preview_thread)->joinable()) {
      return; // Already running
    }

    should_stop->store(false);
    *preview_thread = std::make_unique<std::thread>([=]() {
      // Clone the base game for this thread
      auto game = base_game->clone();

      while (!should_stop->load()) {
        // Get current models (thread-safe)
        std::shared_ptr<std::vector<std::shared_ptr<model::Model<ObsType>>>> available_models;
        {
          std::lock_guard<std::mutex> lock(*models_mutex);
          available_models = *current_models;
        }

        // Skip if no models available
        if (!available_models || available_models->empty()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }

        // Determine how many models the game needs
        size_t needed_players = game->get_player_count();

        // Select models for the game
        auto game_models = std::make_shared<std::vector<std::shared_ptr<model::Model<ObsType>>>>();

        if (available_models->size() >= needed_players) {
          // Use the first N models (training system provides them in preference order)
          for (size_t i = 0; i < needed_players; ++i) {
            game_models->push_back((*available_models)[i]);
          }
        } else {
          // Not enough models - fill with what we have, then duplicate the best
          for (auto &model : *available_models) {
            game_models->push_back(model);
          }

          // Fill remaining slots by duplicating the first (best) model
          while (game_models->size() < needed_players && !available_models->empty()) {
            game_models->push_back((*available_models)[0]);
          }
        }

        // Skip if we still don't have enough models
        if (game_models->size() < needed_players) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }

        // Reset the game with a new seed
        game->init((*game_seed)++);

        // Play and render the game
        try {
          play_and_render(*game, *game_models);
        } catch (const std::exception &e) {
          std::cerr << "Preview error: " << e.what() << std::endl;
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Small delay between games
        if (!should_stop->load()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
      }
    });
  };

  // Stop function - signals the thread to stop and waits for it
  auto stop = [=]() {
    should_stop->store(true);
    if (*preview_thread && (*preview_thread)->joinable()) {
      (*preview_thread)->join();
      preview_thread->reset();
    }
  };

  return std::make_tuple(update_callback, start, stop);
}

// Convenience function that starts immediately and returns stop function
template <typename ObsType>
auto start_preview(std::shared_ptr<Game<ObsType>> base_game) {
  auto [update_callback, start, stop] = create_preview<ObsType>(base_game);
  start();
  return std::make_pair(update_callback, stop);
}

// RAII wrapper for automatic cleanup (optional, if you want automatic stop)
template <typename ObsType>
class PreviewGuard {
  PreviewControl<ObsType> stop_fn_;

public:
  explicit PreviewGuard(PreviewControl<ObsType> stop_fn) : stop_fn_(std::move(stop_fn)) {}
  ~PreviewGuard() {
    if (stop_fn_)
      stop_fn_();
  }

  // Non-copyable but movable
  PreviewGuard(const PreviewGuard &) = delete;
  PreviewGuard &operator=(const PreviewGuard &) = delete;
  PreviewGuard(PreviewGuard &&other) noexcept : stop_fn_(std::move(other.stop_fn_)) {
    other.stop_fn_ = nullptr;
  }
  PreviewGuard &operator=(PreviewGuard &&other) noexcept {
    if (this != &other) {
      if (stop_fn_)
        stop_fn_();
      stop_fn_ = std::move(other.stop_fn_);
      other.stop_fn_ = nullptr;
    }
    return *this;
  }
};
