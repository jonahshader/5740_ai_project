// abstraction over OpenGL for game rendering. mostly written by claude 3.7
#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <functional>
#include <glad/glad.h>
#include <vector>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

/**
 * PixelGame class - Handles SDL initialization, game loop, and OpenGL rendering with aspect ratio
 * preservation
 */
class PixelGame {
private:
  SDL_Window *window = nullptr;
  SDL_GLContext gl_context = nullptr; // OpenGL context
  bool running = false;
  int target_fps;
  int monitor_refresh_rate = 60; // Default refresh rate
  int frame_repeat_count = 1;    // Number of refreshes per game update
  int current_frame = 0;         // Current frame in the repetition cycle

  // Window size tracking
  int window_width = 0;
  int window_height = 0;

  // Viewport tracking for aspect ratio preservation
  int viewport_x = 0;
  int viewport_y = 0;
  int viewport_width = 0;
  int viewport_height = 0;

  // OpenGL related variables
  GLuint shader_program = 0;
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint texture = 0;

  // Initialize OpenGL
  bool init_opengl();

  // Create and compile shader
  GLuint create_shader(GLenum shader_type, const char *shader_source);

  // Create shader program
  bool create_shader_program();

  // Handle window resize
  void handle_resize(int width, int height);

public:
  int internal_width;
  int internal_height;

  /**
   * Constructor - Initialize the PixelGame
   *
   * @param title Window title
   * @param internal_width The internal rendering width
   * @param internal_height The internal rendering height
   * @param initial_window_width The initial window width
   * @param initial_window_height The initial window height
   * @param target_fps Target frames per second
   */
  PixelGame(const std::string &title, int internal_width, int internal_height,
            int initial_window_width, int initial_window_height, int target_fps);

  /**
   * Destructor - Clean up SDL and OpenGL resources
   */
  ~PixelGame();

  /**
   * Run the game loop with provided update, render, and input handling functions
   *
   * @param update_func Function to update game state
   * @param render_func Function to render game state, takes an array of RGBA pixels
   * @param handle_input Function to handle input events, takes SDL_Event
   * @param imgui_update_func Function to update ImGui interface, called every frame
   */
  void run(
      std::function<void()> update_func,
      std::function<void(std::vector<uint32_t> &pixels)> render_func,
      std::function<void(SDL_Event &)> handle_input,
      std::function<void()> imgui_update_func = []() {});

  /**
   * Stop the game loop
   */
  void stop();
};
