#pragma once

#include <functional>

#include <SDL2/SDL.h>

#include "jnb.h"
#include "model.h"

namespace model {

class Keyboard : public SimpleModel {
public:
  Keyboard();

  void forward(const std::vector<float> &observation, std::vector<float> &action) override;
  void mutate(std::mt19937 &rng, float mutation_rate) override;
  void reset() override;
  std::shared_ptr<Model> clone() const override;
  std::string get_name() const override;

  std::function<void(SDL_Event &)> get_input_handler(SDL_KeyCode left, SDL_KeyCode right,
                                                     SDL_KeyCode jump);

private:
  bool left{false};
  bool right{false};
  bool jump{false};
};

} // namespace model
