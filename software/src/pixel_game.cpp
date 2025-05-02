// OpenGL implementation of the PixelGame class
#include "pixel_game.h"
#include <iostream>
#include <tuple>

// Vertex shader source code
const char *vertex_shader_source = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main()
    {
        gl_Position = vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

// Fragment shader source code
const char *fragment_shader_source = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoord;

    uniform sampler2D texture1;

    void main()
    {
        FragColor = texture(texture1, TexCoord);
    }
)";

PixelGame::PixelGame(const std::string &title, int initial_window_width, int initial_window_height,
                     int target_fps) {
  this->window_width = initial_window_width;
  this->window_height = initial_window_height;
  this->target_fps = target_fps;

  // Initialize SDL for windowing and events
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    return;
  }

  // Set OpenGL attributes
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // Create window with OpenGL context - now resizable
  window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            initial_window_width, initial_window_height,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

  if (window == nullptr) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return;
  } else {
    std::cout << "Window created successfully!" << std::endl;
  }

  // Create OpenGL context
  gl_context = SDL_GL_CreateContext(window);
  if (gl_context == nullptr) {
    std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return;
  }

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return;
  }

  // Initialize OpenGL
  if (!init_opengl()) {
    std::cerr << "Failed to initialize OpenGL" << std::endl;
    return;
  }

  // Initialize ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  // Setup ImGui style
  ImGui::StyleColorsDark();

  // Set adaptive VSync if available, fallback to regular VSync
  if (SDL_GL_SetSwapInterval(-1) != 0) {
    SDL_GL_SetSwapInterval(1);
  }

  // Detect monitor refresh rate
  SDL_DisplayMode mode;
  if (SDL_GetWindowDisplayMode(window, &mode) == 0 && mode.refresh_rate != 0) {
    monitor_refresh_rate = mode.refresh_rate;
  } else {
    monitor_refresh_rate = 60; // Default fallback
  }

  // Calculate frame repeat count (round to nearest integer)
  frame_repeat_count = (monitor_refresh_rate + (target_fps / 2)) / target_fps;
  if (frame_repeat_count < 1)
    frame_repeat_count = 1;

  std::cout << "Monitor refresh: " << monitor_refresh_rate << "Hz, Target: " << target_fps
            << "Hz, Frame repeat: " << frame_repeat_count << std::endl;

  running = true;
}

PixelGame::~PixelGame() {
  // Clean up ImGui
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  // Clean up OpenGL resources
  if (shader_program != 0) {
    glDeleteProgram(shader_program);
  }
  if (vao != 0) {
    glDeleteVertexArrays(1, &vao);
  }
  if (vbo != 0) {
    glDeleteBuffers(1, &vbo);
  }
  if (texture != 0) {
    glDeleteTextures(1, &texture);
  }

  // Clean up SDL resources
  if (gl_context != nullptr) {
    SDL_GL_DeleteContext(gl_context);
    gl_context = nullptr;
  }
  if (window != nullptr) {
    SDL_DestroyWindow(window);
    window = nullptr;
  }
  SDL_Quit();
}

bool PixelGame::init_opengl() {
  // Create shader program
  if (!create_shader_program()) {
    return false;
  }

  // Set up vertex data and buffers
  float vertices[] = {
      // positions        // texture coords
      -1.0f, 1.0f,  0.0f, 0.0f, 0.0f, // top left
      1.0f,  1.0f,  0.0f, 1.0f, 0.0f, // top right
      1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, // bottom right
      1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, // bottom right
      -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // bottom left
      -1.0f, 1.0f,  0.0f, 0.0f, 0.0f  // top left
  };

  // Generate and bind VAO and VBO
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Texture coord attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Create texture
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Allocate texture data
  // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, internal_width, internal_height, 0, GL_RGBA,
  //              GL_UNSIGNED_BYTE, nullptr);

  // Use shader program
  glUseProgram(shader_program);

  // Set the texture uniform
  glUniform1i(glGetUniformLocation(shader_program, "texture1"), 0);

  return true;
}

GLuint PixelGame::create_shader(GLenum shader_type, const char *shader_source) {
  GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &shader_source, nullptr);
  glCompileShader(shader);

  // Check for compilation errors
  GLint success;
  GLchar info_log[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, nullptr, info_log);
    std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << info_log << std::endl;
    return 0;
  }

  return shader;
}

bool PixelGame::create_shader_program() {
  // Create vertex and fragment shaders
  GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
  if (vertex_shader == 0) {
    return false;
  }

  GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
  if (fragment_shader == 0) {
    glDeleteShader(vertex_shader);
    return false;
  }

  // Create shader program
  shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  // Check for linking errors
  GLint success;
  GLchar info_log[512];
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
    std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << info_log << std::endl;
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return false;
  }

  // Shaders are linked into the program, so we can delete them
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return true;
}

void PixelGame::run(std::function<void()> update_func,
                    std::function<std::pair<int, int>(std::vector<uint32_t> &pixels)> render_func,
                    std::function<void(SDL_Event &)> handle_input,
                    std::function<void()> imgui_update_func) {

  if (!running) {
    std::cerr << "Cannot run game: not initialized properly" << std::endl;
    return;
  }

  // Create pixel buffer
  std::vector<uint32_t> pixels;
  int internal_width = 0;
  int internal_height = 0;

  // Main game loop
  while (running) {
    // Handle events
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
      // Pass events to ImGui
      ImGui_ImplSDL2_ProcessEvent(&e);

      if (e.type == SDL_QUIT) {
        running = false;
      } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        running = false;
      } else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
        // Handle window resize
        window_width = e.window.data1;
        window_height = e.window.data2;

      } else {
        // Pass other events to the provided input handler
        handle_input(e);
      }
    }

    // Start ImGui frame - ALWAYS do this every frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Always update ImGui interface
    imgui_update_func();

    // Check if we should update the game state and render a new frame
    bool should_update_game = false;
    current_frame = (current_frame + 1) % frame_repeat_count;
    if (current_frame == 0) {
      should_update_game = true;
    }

    // Only update game state and render if needed
    if (should_update_game) {
      // Update game state
      update_func();

      // Clear the pixel buffer
      std::fill(pixels.begin(), pixels.end(), 0);

      // Let the game render to the pixel buffer
      auto [new_internal_width, new_internal_height] = render_func(pixels);

      // Update the texture with the new pixel data
      glBindTexture(GL_TEXTURE_2D, texture);
      // Reallocate texture memory if necessary
      if (new_internal_width != internal_width || new_internal_height != internal_height) {
        // Resize the texture if dimensions have changed
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, new_internal_width, new_internal_height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        internal_width = new_internal_width;
        internal_height = new_internal_height;
      }
      handle_resize(window_width, window_height, internal_width, internal_height);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, internal_width, internal_height, GL_RGBA,
                      GL_UNSIGNED_BYTE, pixels.data());
    }

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Set the viewport to maintain aspect ratio
    glViewport(viewport_x, viewport_y, viewport_width, viewport_height);

    // Draw the texture
    glUseProgram(shader_program);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Swap buffers
    SDL_GL_SwapWindow(window);
  }
}

void PixelGame::handle_resize(int width, int height, int internal_width, int internal_height) {
  // Calculate the aspect ratios
  float window_aspect = (float)width / (float)height;
  float game_aspect = (float)internal_width / (float)internal_height;

  // Calculate the viewport dimensions to maintain aspect ratio
  if (window_aspect > game_aspect) {
    // Window is wider than the game aspect, center horizontally (letterboxing)
    viewport_height = height;
    viewport_width = (int)(height * game_aspect);
    viewport_x = (width - viewport_width) / 2;
    viewport_y = 0;
  } else {
    // Window is taller than the game aspect, center vertically (pillarboxing)
    viewport_width = width;
    viewport_height = (int)(width / game_aspect);
    viewport_x = 0;
    viewport_y = (height - viewport_height) / 2;
  }

  // Store the new window dimensions
  window_width = width;
  window_height = height;
}

void PixelGame::stop() {
  running = false;
}
