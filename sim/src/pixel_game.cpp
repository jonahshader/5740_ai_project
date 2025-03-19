// abstraction over SDL2 for game rendering. mostly written by Claude 3.7
#include "pixel_game.h"
#include <iostream>

PixelGame::PixelGame(const std::string &title, int internal_width, int internal_height,
                     int scale_factor, int target_fps) {

  this->internal_width = internal_width;
  this->internal_height = internal_height;
  this->scale_factor = scale_factor;
  this->target_fps = target_fps;

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    return;
  }

  // Set texture filtering to nearest neighbor
  if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0") == SDL_FALSE) {
    std::cerr << "Warning: Nearest texture filtering not enabled!" << std::endl;
  }

  // Calculate window size based on internal size and scale factor
  int window_width = internal_width * scale_factor;
  int window_height = internal_height * scale_factor;

  // Create window
  window = SDL_CreateWindow(title.c_str(),           // Window title
                            SDL_WINDOWPOS_UNDEFINED, // Initial x position
                            SDL_WINDOWPOS_UNDEFINED, // Initial y position
                            window_width,            // Width
                            window_height,           // Height
                            SDL_WINDOW_SHOWN         // Flags
  );

  if (window == nullptr) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return;
  }

  // Create renderer with hardware acceleration and vsync
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return;
  }

  // Set logical size for automatic scaling
  SDL_RenderSetLogicalSize(renderer, internal_width, internal_height);

  // Create a texture that we'll render to (our "virtual screen")
  render_target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                    internal_width, internal_height);

  if (render_target == nullptr) {
    std::cerr << "Render target could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return;
  }

  running = true;
}

PixelGame::~PixelGame() {
  if (render_target != nullptr) {
    SDL_DestroyTexture(render_target);
    render_target = nullptr;
  }
  if (renderer != nullptr) {
    SDL_DestroyRenderer(renderer);
    renderer = nullptr;
  }
  if (window != nullptr) {
    SDL_DestroyWindow(window);
    window = nullptr;
  }
  SDL_Quit();
}

void PixelGame::run(std::function<void()> update_func,
                    std::function<void(SDL_Renderer *)> render_func,
                    std::function<void(SDL_Event &)> handle_input) {

  if (!running) {
    std::cerr << "Cannot run game: not initialized properly" << std::endl;
    return;
  }

  const int frame_delay = 1000 / target_fps;
  Uint32 frame_start;
  int frame_time;

  // Main game loop
  while (running) {
    frame_start = SDL_GetTicks();

    // Handle events
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        running = false;
      } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        running = false;
      } else {
        // Pass other events to the provided input handler
        handle_input(e);
      }
    }

    // Update game state
    update_func();

    // Render to the texture target
    SDL_SetRenderTarget(renderer, render_target);
    render_func(renderer);

    // Render the texture target to the screen with scaling
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_RenderCopy(renderer, render_target, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    // Cap the frame rate
    frame_time = SDL_GetTicks() - frame_start;
    if (frame_time < frame_delay) {
      SDL_Delay(frame_delay - frame_time);
    }

    // Calculate delta time for next frame
    frame_time = SDL_GetTicks() - frame_start;
  }
}

void PixelGame::stop() {
  running = false;
}