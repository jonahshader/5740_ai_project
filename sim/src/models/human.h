#pragma once

#include <functional>

#include <SDL2/SDL.h>

#include "core.h"
#include "interfaces.h"

namespace jnb {

class HumanModel : public Model {
public:
  HumanModel();

  PlayerInput forward(const GameState &state, bool p1_perspective) override;
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void reset() override;
  std::shared_ptr<Model> clone() const override;

  std::function<void(SDL_Event &)> get_input_handler(SDL_KeyCode left, SDL_KeyCode right, SDL_KeyCode jump);

private:
  PlayerInput input;
};

} // namespace jnb
