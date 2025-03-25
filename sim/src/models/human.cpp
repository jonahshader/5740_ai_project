#include "human.h"

namespace jnb {

HumanModel::HumanModel() : input{} {}

PlayerInput HumanModel::forward(const GameState &state, bool p1_perspective) {
  return input;
}

void HumanModel::mutate(std::mt19937 &rng, float mutation_rate) {}

void HumanModel::reset() {
  // not necessary at all...
  input = PlayerInput{};
}

std::shared_ptr<Model> HumanModel::clone() const {
  return std::make_shared<HumanModel>(*this);
}

std::function<void(SDL_Event &)> HumanModel::get_input_handler(SDL_KeyCode left, SDL_KeyCode right,
                                                               SDL_KeyCode jump) {
  auto handle_input_lambda = [left, right, jump, &input = input](SDL_Event &event) {
    auto k = event.key.keysym.sym;
    if (event.type == SDL_KEYDOWN) {
      if (k == left)
        input.left = true;
      else if (k == right)
        input.right = true;
      else if (k == jump)
        input.jump = true;

    } else if (event.type == SDL_KEYUP) {
      if (k == left)
        input.left = false;
      else if (k == right)
        input.right = false;
      else if (k == jump)
        input.jump = false;
    }
  };
  return handle_input_lambda;
}

} // namespace jnb
