#include <string>
#include <cstring>
#include <memory>
#include <random>

#include "core_render.h"
#include "models/human.h"
#include "models/mlp_simple.h"
#include "models/mlp_map_lut.h"
#include "optimizers/simple.h"
#include "pixel_game.h"

#include "lodepng.h"
#include <verilated.h>
#include "Vgame_test.h"

using namespace jnb;

int main(int argc, char *argv[]) {
  std::string map_file = "jnb_map_tb.tmx"; // default map file

  // pass args to verilated
  Verilated::commandArgs(argc, argv);

  // initialize game state
  GameState state = init(map_file, 0);

  PixelGame game("JnB Sim", CELL_SIZE * state.map.width, CELL_SIZE * state.map.height, 640, 480,
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

  // instantiate Vgame_test
  std::cout << "Instantiating Vgame_test..." << std::endl;
  auto vgame_test = std::make_shared<Vgame_test>();
  std::cout << "done instantiating." << std::endl;

  // init with seed
  vgame_test->clk = 0;
  vgame_test->init = 0;
  vgame_test->swap_start = 0;
  vgame_test->p1_input_left = 0;
  vgame_test->p1_input_right = 0;
  vgame_test->p2_input_jump = 0;
  vgame_test->p2_input_left = 0;
  vgame_test->p2_input_right = 0;
  vgame_test->p2_input_jump = 0;
  vgame_test->go = 0;
  vgame_test->seed = 5;
  std::cout << "before first eval" << std::endl;
  vgame_test->eval();
  std::cout << "after first eval" << std::endl;
  // run until we see done is high (meaning we can go ahead and init)
  while (vgame_test->done == 0) {
    vgame_test->clk = 1;
    vgame_test->eval();
    vgame_test->clk = 0;
    vgame_test->eval();
    std::cout << "first eval loop" << std::endl;
  }
  // init
  vgame_test->init = 1;
  vgame_test->clk = 1;
  vgame_test->eval();
  vgame_test->init = 0;
  vgame_test->clk = 0;
  vgame_test->eval();
  // run until we get done
  while (vgame_test->done == 0) {
    vgame_test->clk = 1;
    vgame_test->eval();
    vgame_test->clk = 0;
    vgame_test->eval();
    std::cout << "second eval loop" << std::endl;
    std::cout << vgame_test->done << std::endl;
  }
  // vgame_test is ready

  // make human players
  auto model1 = std::make_shared<HumanModel>();
  auto model2 = std::make_shared<HumanModel>();

  auto update_lambda = [&state, model1, model2, vgame_test]() {
    auto p1_input = model1->forward(state, true);
    auto p2_input = model2->forward(state, false);

    // set input
    vgame_test->p1_input_left = p1_input.left;
    vgame_test->p1_input_right = p1_input.right;
    vgame_test->p1_input_jump = p1_input.jump;
    vgame_test->p2_input_left = p2_input.left;
    vgame_test->p2_input_right = p2_input.right;
    vgame_test->p2_input_jump = p2_input.jump;
    // go
    vgame_test->go = 1;
    vgame_test->clk = 1;
    vgame_test->eval();
    vgame_test->clk = 0;
    vgame_test->eval();
    // run until done is true
    while (vgame_test->done == 0) {
      vgame_test->clk = 1;
      vgame_test->eval();
      vgame_test->clk = 0;
      vgame_test->eval();
    }
    // update game state based on vgame_test outputs
    state.p1.x = F4(static_cast<int16_t>(vgame_test->p1_x));
    state.p1.y = F4(static_cast<int16_t>(vgame_test->p1_y));
    state.p2.x = F4(static_cast<int16_t>(vgame_test->p2_x));
    state.p2.y = F4(static_cast<int16_t>(vgame_test->p2_y));
    state.p1.score = vgame_test->p1_score;
    state.p2.score = vgame_test->p2_score;
    state.age = vgame_test->age;
    // TODO: update coin position (forgot this in vhdl)

    // update(state, p1_input, p2_input);
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
  // input_handlers.push_back([&state, &seed](SDL_Event &event) {
  //   auto k = event.key.keysym.sym;
  //   if (event.type == SDL_KEYDOWN) {
  //     if (k == SDLK_r) {
  //       reinit(state, ++seed);
  //     }
  //   }
  // });

  auto handle_input_lambda = [&](SDL_Event &event) {
    // run all input handlers
    for (auto &handler : input_handlers) {
      handler(event);
    }
  };

  game.run(update_lambda, render_lambda, handle_input_lambda);

  // cleanup vgame_test
  vgame_test->final();

  return 0;
}
