#include "core_render.h"

#include <iostream>
#include <memory>

#include "lodepng.h"

#include "interfaces.h"
#include "models/human.h"

// constexpr int asdf = 0;

namespace jnb {

uint32_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | r;
  return rgba;
}

void blit_tile(SDL_Renderer *renderer, const std::vector<uint8_t> &spritesheet, int x_tile,
               int y_tile, int t_id) {
  for (int y = 0; y < CELL_SIZE; ++y) {
    for (int x = 0; x < CELL_SIZE; ++x) {
      const uint8_t *rgb = &spritesheet[4 * (x + (y + t_id * CELL_SIZE) * CELL_SIZE)];
      SDL_SetRenderDrawColor(renderer, rgb[0], rgb[1], rgb[2], 255);
      SDL_RenderDrawPoint(renderer, x + x_tile * CELL_SIZE, y + y_tile * CELL_SIZE);
    }
  }
}

void blit_tile(std::vector<uint32_t> &pixels, const TileMap &map,
               const std::vector<uint8_t> &spritesheet, int x_tile, int y_tile, int t_id) {
  for (int y = 0; y < CELL_SIZE; ++y) {
    for (int x = 0; x < CELL_SIZE; ++x) {
      const uint8_t *rgb = &spritesheet[4 * (x + (y + t_id * CELL_SIZE) * CELL_SIZE)];
      size_t index = (x + x_tile * CELL_SIZE) + (y + y_tile * CELL_SIZE) * map.width * CELL_SIZE;
      pixels[index] = make_color(rgb[0], rgb[1], rgb[2], 255);
    }
  }
}

void render(const GameState &state, SDL_Renderer *renderer,
            const std::vector<uint8_t> &spritesheet) {
  // draw background
  for (int y_tile = 0; y_tile < state.map.height; ++y_tile) {
    for (int x_tile = 0; x_tile < state.map.width; ++x_tile) {
      const uint8_t t_id = state.map.tiles[y_tile][x_tile] - 1;
      blit_tile(renderer, spritesheet, x_tile, y_tile, t_id);
    }
  }

  // draw coin
  blit_tile(renderer, spritesheet, state.coin_pos.x, state.map.height - state.coin_pos.y - 1,
            static_cast<int>(Tile::COIN) - 1);

  // draw players
  // p1 is light red
  if (state.p1.dead_timeout == 0) {
    SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
    int x1 = state.p1.x.to_integer_floor();
    int y1 = state.map.height * CELL_SIZE - state.p1.y.to_integer_floor() - PLAYER_HEIGHT;
    SDL_Rect r{x1, y1, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_RenderFillRect(renderer, &r);
  }

  // p2 is light blue
  if (state.p2.dead_timeout == 0) {
    SDL_SetRenderDrawColor(renderer, 80, 80, 255, 255);
    int x1 = state.p2.x.to_integer_floor();
    int y1 = state.map.height * CELL_SIZE - state.p2.y.to_integer_floor() - PLAYER_HEIGHT;
    SDL_Rect r{x1, y1, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_RenderFillRect(renderer, &r);
  }

  // draw score
  SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
  for (int i = 0; i < state.p1.score; ++i) {
    SDL_RenderDrawPoint(renderer, i, 0);
  }
  SDL_SetRenderDrawColor(renderer, 80, 80, 255, 255);
  for (int i = 0; i < state.p2.score; ++i) {
    SDL_RenderDrawPoint(renderer, state.map.width * CELL_SIZE - i - 1, 1);
  }
}

void render(const GameState &state, const std::vector<uint8_t> &spritesheet,
            std::vector<uint32_t> &pixels) {
  // draw background
  for (int y_tile = 0; y_tile < state.map.height; ++y_tile) {
    for (int x_tile = 0; x_tile < state.map.width; ++x_tile) {
      const uint8_t t_id = state.map.tiles[y_tile][x_tile] - 1;
      blit_tile(pixels, state.map, spritesheet, x_tile, y_tile, t_id);
    }
  }

  // draw coin
  blit_tile(pixels, state.map, spritesheet, state.coin_pos.x,
            state.map.height - state.coin_pos.y - 1, static_cast<int>(Tile::COIN) - 1);

  // draw players
  // p1 is light red
  int32_t p1_col = make_color(255, 80, 80, 255);
  if (state.p1.dead_timeout == 0) {
    int x1 = state.p1.x.to_integer_floor();
    int y1 = state.map.height * CELL_SIZE - state.p1.y.to_integer_floor() - PLAYER_HEIGHT;

    for (int y = 0; y < PLAYER_HEIGHT; ++y) {
      for (int x = 0; x < PLAYER_WIDTH; ++x) {
        int px = x1 + x;
        int py = y1 + y;
        size_t index = px + py * state.map.width * CELL_SIZE;
        pixels[index] = p1_col;
      }
    }
  }

  // p2 is light blue
  int32_t p2_col = make_color(80, 80, 255, 255);
  if (state.p2.dead_timeout == 0) {
    int x1 = state.p2.x.to_integer_floor();
    int y1 = state.map.height * CELL_SIZE - state.p2.y.to_integer_floor() - PLAYER_HEIGHT;

    for (int y = 0; y < PLAYER_HEIGHT; ++y) {
      for (int x = 0; x < PLAYER_WIDTH; ++x) {
        int px = x1 + x;
        int py = y1 + y;
        size_t index = px + py * state.map.width * CELL_SIZE;
        pixels[index] = p2_col;
      }
    }
  }

  // draw score
  for (int i = 0; i < std::min(state.p1.score, state.map.width * CELL_SIZE); ++i) {
    pixels[i] = p1_col;
  }
  for (int i = 0; i < std::min(state.p2.score, state.map.width * CELL_SIZE); ++i) {
    pixels[(((state.map.width) * CELL_SIZE) * 2 - i - 1)] = p2_col;
  }
}

void run_game(const std::string &map_filename, uint64_t seed) {
  // initialize game state
  std::shared_ptr<GameState> state = std::make_shared<GameState>(init(map_filename, seed));
  // initialize player input states
  std::shared_ptr<PlayerInput> p1_input = std::make_shared<PlayerInput>();
  std::shared_ptr<PlayerInput> p2_input = std::make_shared<PlayerInput>();

  PixelGame game("JnB Sim", state->map.width * CELL_SIZE, state->map.height * CELL_SIZE, 640, 480,
                 60);

  std::vector<uint8_t> spritesheet;
  uint32_t w, h;
  auto error = lodepng::decode(spritesheet, w, h, "tiles.png"); // TODO: move path to constexpr
  if (error) {
    std::cerr << "Error loading spritesheet: " << lodepng_error_text(error) << std::endl;
    throw std::runtime_error("Failed to load spritesheet");
  }
  std::cout << "Loaded tiles.png" << std::endl;
  std::cout << "Width: " << w << ", Height: " << h << std::endl;

  auto update_lambda = [=]() { update(*state, *p1_input, *p2_input); };

  auto handle_input_lambda = [=](SDL_Event &event) {
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_LEFT:
          p1_input->left = true;
          break;
        case SDLK_RIGHT:
          p1_input->right = true;
          break;
        case SDLK_UP:
          p1_input->jump = true;
          break;
        case SDLK_a:
          p2_input->left = true;
          break;
        case SDLK_d:
          p2_input->right = true;
          break;
        case SDLK_w:
          p2_input->jump = true;
          break;
      }
    } else if (event.type == SDL_KEYUP) {
      switch (event.key.keysym.sym) {
        case SDLK_LEFT:
          p1_input->left = false;
          break;
        case SDLK_RIGHT:
          p1_input->right = false;
          break;
        case SDLK_UP:
          p1_input->jump = false;
          break;
        case SDLK_a:
          p2_input->left = false;
          break;
        case SDLK_d:
          p2_input->right = false;
          break;
        case SDLK_w:
          p2_input->jump = false;
          break;
      }
    }
  };

  // auto render_lambda = [=](SDL_Renderer *renderer) { render(*state, renderer, spritesheet); };
  auto render_lambda = [=](std::vector<uint32_t> &pixels) { render(*state, spritesheet, pixels); };

  game.run(update_lambda, render_lambda, handle_input_lambda);
}

void run_game_with_models(const std::string &map_filename, uint64_t seed,
                          std::shared_ptr<Model> model1, std::shared_ptr<Model> model2) {
  // initialize game state
  GameState state = init(map_filename, seed);

  PixelGame game("JnB Sim", state.map.width * CELL_SIZE, state.map.height * CELL_SIZE, 640, 480,
                 60);

  std::vector<uint8_t> spritesheet;
  uint32_t w, h;
  auto error = lodepng::decode(spritesheet, w, h, "tiles.png"); // TODO: move path to constexpr
  if (error) {
    std::cerr << "Error loading spritesheet: " << lodepng_error_text(error) << std::endl;
    throw std::runtime_error("Failed to load spritesheet");
  }
  std::cout << "Loaded tiles.png" << std::endl;
  std::cout << "Width: " << w << ", Height: " << h << std::endl;

  auto update_lambda = [&state, model1, model2]() {
    auto p1_input = model1->forward(state, true);
    auto p2_input = model2->forward(state, false);
    update(state, p1_input, p2_input);
  };

  // auto render_lambda = [&state, &spritesheet](SDL_Renderer *renderer) {
  //   render(state, renderer, spritesheet);
  // };
  auto render_lambda = [&state, &spritesheet](std::vector<uint32_t> &pixels) {
    render(state, spritesheet, pixels);
  };

  std::vector<std::function<void(SDL_Event &)>> input_handlers;

  // try downcasting each model to HumanModel.
  // if we can, then grab it's input handler
  // and add it to the input_handlers vector
  auto human_model1 = std::dynamic_pointer_cast<HumanModel>(model1);
  if (human_model1) {
    auto handle_input_lambda = human_model1->get_input_handler(SDLK_LEFT, SDLK_RIGHT, SDLK_UP);
    input_handlers.push_back(handle_input_lambda);
  }
  auto human_model2 = std::dynamic_pointer_cast<HumanModel>(model2);
  if (human_model2) {
    auto handle_input_lambda = human_model2->get_input_handler(SDLK_a, SDLK_d, SDLK_w);
    input_handlers.push_back(handle_input_lambda);
  }
  input_handlers.push_back([&state, &seed](SDL_Event &event) {
    auto k = event.key.keysym.sym;
    if (event.type == SDL_KEYDOWN) {
      if (k == SDLK_r) {
        reinit(state, ++seed);
      }
    }
  });

  auto handle_input_lambda = [&](SDL_Event &event) {
    // run all input handlers
    for (auto &handler : input_handlers) {
      handler(event);
    }
  };

  game.run(update_lambda, render_lambda, handle_input_lambda);
}

} // namespace jnb
