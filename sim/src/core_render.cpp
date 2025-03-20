#include "core_render.h"

#include <iostream>
#include <memory>

#include "lodepng.h"

// constexpr int asdf = 0;

namespace jnb {
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

void render(const GameState &state, SDL_Renderer *renderer,
            const std::vector<uint8_t> &spritesheet) {
  // draw background
  for (int y_tile = 0; y_tile < HEIGHT_CELLS; ++y_tile) {
    for (int x_tile = 0; x_tile < WIDTH_CELLS; ++x_tile) {
      const uint8_t t_id = base_map[y_tile][x_tile] - 1;
      blit_tile(renderer, spritesheet, x_tile, y_tile, t_id);
    }
  }

  // draw coin
  blit_tile(renderer, spritesheet, state.coin_pos.x, HEIGHT_CELLS - state.coin_pos.y - 1,
            static_cast<int>(Tile::COIN) - 1);

  // draw players
  // p1 is light red
  SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
  int x1 = state.p1.x.to_integer_floor();
  int y1 = HEIGHT_CELLS * CELL_SIZE - state.p1.y.to_integer_floor() - PLAYER_HEIGHT;
  SDL_Rect r{x1, y1, PLAYER_WIDTH, PLAYER_HEIGHT};
  SDL_RenderFillRect(renderer, &r);
  // p2 is light blue
  SDL_SetRenderDrawColor(renderer, 80, 80, 255, 255);
  x1 = state.p2.x.to_integer_floor();
  y1 = HEIGHT_CELLS * CELL_SIZE - state.p2.y.to_integer_floor() - PLAYER_HEIGHT;
  r.x = x1;
  r.y = y1;
  SDL_RenderFillRect(renderer, &r);

  // draw score
  SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
  for (int i = 0; i < state.p1.score; ++i) {
    SDL_RenderDrawPoint(renderer, i, 0);
  }
  SDL_SetRenderDrawColor(renderer, 80, 80, 255, 255);
  for (int i = 0; i < state.p2.score; ++i) {
    SDL_RenderDrawPoint(renderer, WIDTH_CELLS * CELL_SIZE - i - 1, 1);
  }
}

void run_game(uint64_t seed) {
  PixelGame game("JnB Sim", CELL_SIZE * WIDTH_CELLS, CELL_SIZE * HEIGHT_CELLS, 16, 60);

  std::shared_ptr<GameState> state = std::make_shared<GameState>(init(seed));
  std::shared_ptr<PlayerInput> p1_input = std::make_shared<PlayerInput>();
  std::shared_ptr<PlayerInput> p2_input = std::make_shared<PlayerInput>();

  std::vector<uint8_t> spritesheet;
  uint32_t w, h;
  auto error = lodepng::decode(spritesheet, w, h, "tiles.png"); // TODO: move path to constexpr
  if (error) {
    std::cerr << "Error loading spritesheet: " << lodepng_error_text(error) << std::endl;
    throw std::runtime_error("Failed to load spritesheet");
  }
  std::cout << "Loaded tiles.png" << std::endl;
  std::cout << "Width: " << w << ", Height: " << h << std::endl;

  auto update_lambda = [=]() {
    update(*state, *p1_input, *p2_input);
  };

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

  auto render_lambda = [=](SDL_Renderer *renderer) {
    render(*state, renderer, spritesheet);
  };

  game.run(update_lambda, render_lambda, handle_input_lambda);
}
} // namespace jnb
