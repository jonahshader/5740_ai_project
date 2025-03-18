#include <SDL2/SDL.h>
#include <iostream>
#include <functional>
#include <string>

/**
 * PixelGame class - Handles SDL initialization, game loop, and rendering with scaling
 */
class PixelGame {
private:
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *renderTarget = nullptr;
  bool running = false;
  int targetFPS;
  float deltaTime = 0.0f;

public:
  int internalWidth;
  int internalHeight;
  int scaleFactor;

  /**
   * Initialize the PixelGame
   *
   * @param title Window title
   * @param internalWidth The internal rendering width
   * @param internalHeight The internal rendering height
   * @param scaleFactor How much to scale up by
   * @param targetFPS Target frames per second
   * @return true if initialization succeeded
   */
  bool init(const std::string &title, int internalWidth, int internalHeight, int scaleFactor,
            int targetFPS) {

    this->internalWidth = internalWidth;
    this->internalHeight = internalHeight;
    this->scaleFactor = scaleFactor;
    this->targetFPS = targetFPS;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
      return false;
    }

    // Set texture filtering to nearest neighbor
    if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0") == SDL_FALSE) {
      std::cerr << "Warning: Nearest texture filtering not enabled!" << std::endl;
    }

    // Calculate window size based on internal size and scale factor
    int windowWidth = internalWidth * scaleFactor;
    int windowHeight = internalHeight * scaleFactor;

    // Create window
    window = SDL_CreateWindow(title.c_str(),           // Window title
                              SDL_WINDOWPOS_UNDEFINED, // Initial x position
                              SDL_WINDOWPOS_UNDEFINED, // Initial y position
                              windowWidth,             // Width
                              windowHeight,            // Height
                              SDL_WINDOW_SHOWN         // Flags
    );

    if (window == nullptr) {
      std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
      cleanup();
      return false;
    }

    // Create renderer with hardware acceleration and vsync
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
      std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
      cleanup();
      return false;
    }

    // Set logical size for automatic scaling
    SDL_RenderSetLogicalSize(renderer, internalWidth, internalHeight);

    // Create a texture that we'll render to (our "virtual screen")
    renderTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                     internalWidth, internalHeight);

    if (renderTarget == nullptr) {
      std::cerr << "Render target could not be created! SDL_Error: " << SDL_GetError() << std::endl;
      cleanup();
      return false;
    }

    running = true;
    return true;
  }

  /**
   * Run the game loop with provided update and render functions
   *
   * @param updateFunc Function to update game state, takes deltaTime
   * @param renderFunc Function to render game state, takes the renderer and renderTarget
   */
  void run(std::function<void(float)> updateFunc,
           std::function<void(SDL_Renderer *, SDL_Texture *)> renderFunc) {

    if (!running) {
      std::cerr << "Cannot run game: not initialized properly" << std::endl;
      return;
    }

    const int frameDelay = 1000 / targetFPS;
    Uint32 frameStart;
    int frameTime;

    // Main game loop
    while (running) {
      frameStart = SDL_GetTicks();

      // Handle events
      SDL_Event e;
      while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
          running = false;
        } else if (e.type == SDL_KEYDOWN) {
          if (e.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
          }
        }
      }

      // Update game state
      updateFunc(deltaTime);

      // Render to the texture target
      SDL_SetRenderTarget(renderer, renderTarget);
      renderFunc(renderer, renderTarget);

      // Render the texture target to the screen with scaling
      SDL_SetRenderTarget(renderer, nullptr);
      SDL_RenderCopy(renderer, renderTarget, nullptr, nullptr);
      SDL_RenderPresent(renderer);

      // Cap the frame rate
      frameTime = SDL_GetTicks() - frameStart;
      if (frameTime < frameDelay) {
        SDL_Delay(frameDelay - frameTime);
      }

      // Calculate delta time for next frame
      frameTime = SDL_GetTicks() - frameStart;
      deltaTime = frameTime / 1000.0f;
    }
  }

  /**
   * Clean up SDL resources
   */
  void cleanup() {
    if (renderTarget != nullptr) {
      SDL_DestroyTexture(renderTarget);
      renderTarget = nullptr;
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

  /**
   * Stop the game loop
   */
  void stop() {
    running = false;
  }

  /**
   * Destructor
   */
  ~PixelGame() {
    cleanup();
  }
};

/**
 * Example usage of the PixelGame class
 */
int main(int argc, char *argv[]) {
  // Create game instance
  PixelGame game;

  // Game configuration
  const int INTERNAL_WIDTH = 160;
  const int INTERNAL_HEIGHT = 120;
  const int SCALE_FACTOR = 4;
  const int TARGET_FPS = 60;

  // Game state variables
  float playerX = INTERNAL_WIDTH / 2;
  float playerY = INTERNAL_HEIGHT / 2;
  float angle = 0.0f;

  // Initialize game
  if (!game.init("Pixelated Game", INTERNAL_WIDTH, INTERNAL_HEIGHT, SCALE_FACTOR, TARGET_FPS)) {
    return 1;
  }

  // Define update function
  auto update = [&](float deltaTime) {
    // Move player in a circle
    angle += deltaTime;
    playerX = INTERNAL_WIDTH / 2 + cos(angle) * 30;
    playerY = INTERNAL_HEIGHT / 2 + sin(angle) * 20;
  };

  // Define render function
  auto render = [&](SDL_Renderer *renderer, SDL_Texture *renderTarget) {
    // Clear screen with dark blue
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);

    // Draw a grid
    SDL_SetRenderDrawColor(renderer, 40, 60, 100, 255);
    for (int y = 0; y < INTERNAL_HEIGHT; y += 10) {
      for (int x = 0; x < INTERNAL_WIDTH; x += 10) {
        SDL_RenderDrawPoint(renderer, x, y);
      }
    }

    // Draw player (a 3x3 yellow square)
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    for (int y = -1; y <= 1; y++) {
      for (int x = -1; x <= 1; x++) {
        SDL_RenderDrawPoint(renderer, (int)playerX + x, (int)playerY + y);
      }
    }
  };

  // Run the game
  game.run(update, render);

  return 0;
}
