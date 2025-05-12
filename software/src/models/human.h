#pragma once

#include <functional>
#include <random>

#include <SDL2/SDL.h>

#include "jnb.h"
#include "model.h"

// TODO: make this more general.
// it should take in a list of keys and associated action values.
// could also tack on mouse support.
// also a min and max for each value.
namespace model {

template <typename ObsType> class Keyboard : public Model<ObsType> {
public:
  Keyboard() {}

  void forward(const ObsType &observation, std::vector<float> &action) override {
    action[0] = left ? 1 : 0;
    action[1] = right ? 1 : 0;
    action[2] = jump ? 1 : 0;
  }

  std::shared_ptr<Model<ObsType>> clone() const override {
    return std::make_shared<Keyboard>(*this);
  }
  std::string get_name() const override {
    return "Keyboard";
  }

  std::function<void(SDL_Event &)> get_input_handler(SDL_KeyCode left, SDL_KeyCode right,
                                                     SDL_KeyCode jump) {
    auto handle_input_lambda = [=](SDL_Event &event) {
      auto k = event.key.keysym.sym;
      if (event.type == SDL_KEYDOWN) {
        if (k == left)
          this->left = true;
        else if (k == right)
          this->right = true;
        else if (k == jump)
          this->jump = true;

      } else if (event.type == SDL_KEYUP) {
        if (k == left)
          this->left = false;
        else if (k == right)
          this->right = false;
        else if (k == jump)
          this->jump = false;
      }
    };
    return handle_input_lambda;
  }

private:
  bool left{false};
  bool right{false};
  bool jump{false};
};

} // namespace model
