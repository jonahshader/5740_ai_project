#include "play.h"

#include "pixel_game.h"
#include "models/human.h"

#include <cassert>

std::vector<int> play(Game &game, const std::vector<std::shared_ptr<model::SimpleModel>> &models) {
  assert(game.get_player_count() == models.size());

  // build io vectors, fitness vector
  std::vector<std::vector<float>> inputs, outputs;
  std::vector<int> fitness;
  inputs.resize(game.get_player_count());
  outputs.resize(game.get_player_count());
  fitness.resize(game.get_player_count());
  for (size_t i = 0; i < game.get_player_count(); ++i) {
    inputs[i].resize(game.get_observation_size(), 0.0f);
    outputs[i].resize(game.get_action_count());
  }

  // run the game until done
  while (!game.is_done()) {
    // observe the game state
    game.observe(inputs);

    // run the models
    for (size_t i = 0; i < game.get_player_count(); ++i) {
      models[i]->forward(inputs[i], outputs[i]);
    }

    // update the game with the actions
    game.update(outputs);
  }

  // get the fitness
  game.get_fitness(fitness);
  return fitness;
}

std::vector<int> play_and_render(Game &game, const std::vector<std::shared_ptr<model::SimpleModel>> &models) {
  assert(game.get_player_count() == models.size());

  // build io vectors, fitness vector
  std::vector<std::vector<float>> inputs, outputs;
  std::vector<int> fitness;
  inputs.resize(game.get_player_count());
  outputs.resize(game.get_player_count());
  fitness.resize(game.get_player_count());
  for (size_t i = 0; i < game.get_player_count(); ++i) {
    inputs[i].resize(game.get_observation_size(), 0.0f);
    outputs[i].resize(game.get_action_count());
  }

  PixelGame window(game.get_name(), 640, 480, 60);

  auto update_lambda = [&]() {
    // observe the game state
    game.observe(inputs);

    // run the models
    for (size_t i = 0; i < game.get_player_count(); ++i) {
      models[i]->forward(inputs[i], outputs[i]);
    }

    // update the game with the actions
    game.update(outputs);

    // stop if done
    if (game.is_done()) {
      window.stop();
    }
  };

  auto render_lambda = [&](std::vector<uint32_t> &pixels) {
    const auto res = game.get_resolution();
    pixels.resize(res.first * res.second);
    game.render(pixels);
    return res;
  };

  std::vector<std::function<void(SDL_Event &)>> input_handlers;
  // try downcasting each model to Keyboard.
  // if we can, then grab it's input handler
  // and add it to the input_handlers vector
  size_t human_model_count = 0;
  for (auto &model : models) {
    auto kb_model = std::dynamic_pointer_cast<model::Keyboard>(model);
    if (kb_model) {
      switch (human_model_count) {
        case 0:
          // player 0 uses arrow keys
          input_handlers.push_back(kb_model->get_input_handler(SDLK_LEFT, SDLK_RIGHT, SDLK_UP));
          break;
        case 1:
          // player 1 uses wasd
          input_handlers.push_back(kb_model->get_input_handler(SDLK_a, SDLK_d, SDLK_w));
          break;
        default:
          // currently don't support more than 2 human players
          break;
      }
      ++human_model_count;
    }
  }

  auto handle_input_lambda = [&](SDL_Event &event) {
    // run all input handlers
    for (auto &handler : input_handlers) {
      handler(event);
    }
  };

  window.run(update_lambda, render_lambda, handle_input_lambda);

  game.get_fitness(fitness);
  return fitness;
}
