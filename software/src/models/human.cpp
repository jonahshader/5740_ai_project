#include "human.h"

namespace jnb {

HumanModel::HumanModel() : input{} {}

void HumanModel::forward(const std::vector<float> &observation, std::vector<float> &action) {
  action[0] = input.left ? 1 : 0;
  action[1] = input.right ? 1 : 0;
  action[2] = input.jump ? 1 : 0;
}

void HumanModel::mutate(std::mt19937 &rng, float mutation_rate) {}

void HumanModel::reset() {
  // not necessary at all...
  input = PlayerInput{};
}

std::shared_ptr<Model> HumanModel::clone() const {
  return std::make_shared<HumanModel>(*this);
}

std::string HumanModel::get_name() const {
  return "HumanModel";
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
