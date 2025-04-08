// abstraction over SDL2 for game rendering. mostly written by Claude 3.7
#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <functional>

/**
 * PixelGame class - Handles SDL initialization, game loop, and rendering with scaling
 */
class PixelGame {
private:
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *render_target = nullptr;
  bool running = false;
  int target_fps;

public:
  int internal_width;
  int internal_height;
  int scale_factor;

  /**
   * Constructor - Initialize the PixelGame
   *
   * @param title Window title
   * @param internal_width The internal rendering width
   * @param internal_height The internal rendering height
   * @param scale_factor How much to scale up by
   * @param target_fps Target frames per second
   */
  PixelGame(const std::string &title, int internal_width, int internal_height, int scale_factor,
            int target_fps);

  /**
   * Destructor - Clean up SDL resources
   */
  ~PixelGame();

  /**
   * Run the game loop with provided update, render, and input handling functions
   *
   * @param update_func Function to update game state
   * @param render_func Function to render game state, takes the renderer
   * @param handle_input Function to handle input events, takes SDL_Event
   */
  void run(std::function<void()> update_func, std::function<void(SDL_Renderer *)> render_func,
           std::function<void(SDL_Event &)> handle_input);

  /**
   * Stop the game loop
   */
  void stop();
};