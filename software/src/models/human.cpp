#include "human.h"

namespace model {

Keyboard::Keyboard() {}

void Keyboard::forward(const std::vector<float> &observation, std::vector<float> &action) {
  action[0] = left ? 1 : 0;
  action[1] = right ? 1 : 0;
  action[2] = jump ? 1 : 0;
}

void Keyboard::mutate(std::mt19937 &rng, float mutation_rate) {}

void Keyboard::reset() {}

std::shared_ptr<Model> Keyboard::clone() const {
  return std::make_shared<Keyboard>(*this);
}

std::string Keyboard::get_name() const {
  return "Human";
}

std::function<void(SDL_Event &)> Keyboard::get_input_handler(SDL_KeyCode left, SDL_KeyCode right,
                                                             SDL_KeyCode jump) {
  auto handle_input_lambda = [&](SDL_Event &event) {
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

} // namespace model
