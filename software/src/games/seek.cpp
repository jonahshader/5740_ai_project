#include "seek.h"

#include "color.h"
#include "rendering.h"

#include <cassert>
#include <iostream>

// DiscreteSeek::DiscreteSeek(const Config &config) : config(config) {

// }

ContinuousSeek::ContinuousSeek(const Config &config) : config(config) {}

void ContinuousSeek::init(uint64_t seed) {
  rng = std::mt19937(seed);

  // pick initial locations
  std::uniform_real_distribution<float> player_x_dist(-config.world_width / 2,
                                                      config.world_width / 2);
  std::uniform_real_distribution<float> player_y_dist(-config.world_height / 2,
                                                      config.world_height / 2);
  x = player_x_dist(rng);
  y = player_y_dist(rng);
  // coin must be in different quadrant of map
  std::bernoulli_distribution quadrant_dist{0.5f};
  bool player_x_quad = x >= 0;
  bool player_y_quad = y >= 0;
  bool coin_x_quad = quadrant_dist(rng);
  bool coin_y_quad = quadrant_dist(rng);
  if (player_x_quad == coin_x_quad && player_y_quad == coin_y_quad) {
    // place in opposing quadrant
    coin_x_quad = !coin_x_quad;
    coin_y_quad = !coin_x_quad;
  }
  float left = -config.world_width / 2;
  float right = config.world_width / 2;
  float top = config.world_height / 2;
  float bottom = -config.world_height / 2;
  std::uniform_real_distribution<float> coin_x_dist((coin_x_quad ? 0 : left) + config.coin_radius,
                                                    (coin_x_quad ? right : 0) - config.coin_radius);
  std::uniform_real_distribution<float> coin_y_dist((coin_y_quad ? 0 : bottom) + config.coin_radius,
                                                    (coin_y_quad ? top : 0) - config.coin_radius);
  x_coin = coin_x_dist(rng);
  y_coin = coin_y_dist(rng);

  x_vel = 0;
  y_vel = 0;
  coins_collected = 0;
  // previous coin position is used to measure agent's progress toward obtaining the coin.
  // for this purpose, the previous coin position should be init to the player's spawn.
  px_coin = x;
  py_coin = y;
  timestep = 0;
}

void ContinuousSeek::update(const std::vector<std::vector<float>> &actions) {
  assert(actions.size() == 1); // just 1 player for now
  const auto &act = actions[0];

  float x_accel = std::tanhf(act[0]) * config.max_accel - x_vel * config.friction;
  float y_accel = std::tanhf(act[1]) * config.max_accel - y_vel * config.friction;
  x_vel += x_accel * config.dt;
  y_vel += y_accel * config.dt;
  x += x_vel * config.dt;
  y += y_vel * config.dt;

  // check coin collision
  float dx = x_coin - x;
  float dy = y_coin - y;
  if (dx * dx + dy * dy <= config.coin_radius * config.coin_radius) {
    coins_collected++;
    px_coin = x_coin;
    py_coin = y_coin;

    // pick new coin location
    // must be different than current quadrant
    std::bernoulli_distribution quadrant_dist{0.5f};
    bool curr_x_quad = x_coin >= 0;
    bool curr_y_quad = y_coin >= 0;
    bool coin_x_quad = quadrant_dist(rng);
    bool coin_y_quad = quadrant_dist(rng);
    if (curr_x_quad == coin_x_quad && curr_y_quad == coin_y_quad) {
      // place in opposing quadrant
      coin_x_quad = !coin_x_quad;
      coin_y_quad = !coin_y_quad;
    }
    float left = -config.world_width / 2;
    float right = config.world_width / 2;
    float top = config.world_height / 2;
    float bottom = -config.world_height / 2;
    std::uniform_real_distribution<float> coin_x_dist((coin_x_quad ? 0 : left) + config.coin_radius,
                                                      (coin_x_quad ? right : 0) -
                                                          config.coin_radius);
    std::uniform_real_distribution<float> coin_y_dist((coin_y_quad ? 0 : bottom) +
                                                          config.coin_radius,
                                                      (coin_y_quad ? top : 0) - config.coin_radius);
    x_coin = coin_x_dist(rng);
    y_coin = coin_y_dist(rng);
  }

  ++timestep;
}

void ContinuousSeek::get_fitness(std::vector<int32_t> &fitness) {
  fitness.resize(get_player_count());

  float dx = x_coin - px_coin;
  float dy = y_coin - py_coin;
  float coin_spawn_dist = std::sqrt(dx * dx + dy * dy);
  dx = x_coin - x;
  dy = y_coin - y;
  float player_coin_dist = std::sqrt(dx * dx + dy * dy);
  float progress_towards_coin = (coin_spawn_dist - player_coin_dist) / coin_spawn_dist;
  float fit = progress_towards_coin + coins_collected;
  // std::cout << progress_towards_coin << std::endl;
  // std::cout << x << ", " << y << ", " << x_coin << ", " << y_coin << ", " << coins_collected <<
  // ", "
  //           << fit << std::endl;
  fitness[0] = static_cast<int32_t>(fit * 100.0f); // scale to integer fitness
}

bool ContinuousSeek::is_done() {
  return timestep >= config.frame_limit and config.frame_limit >= 0;
}

void ContinuousSeek::observe(std::vector<obs::Simple> &inputs) {
  inputs.resize(get_player_count());
  auto &input = inputs[0];
  input.clear();

  float x_norm = 1.0f / config.world_width;
  float y_norm = 1.0f / config.world_height;
  // TODO: tune these
  float x_vel_norm = 1.0f / 2.0f;
  float y_vel_norm = 1.0f / 2.0f;

  if (config.absolute_inputs) {
    input.push_back(x * x_norm);
    input.push_back(y * y_norm);
    input.push_back(x_coin * x_norm);
    input.push_back(y_coin * y_norm);
  }
  if (config.relative_inputs) {
    input.push_back((x_coin - x) * x_norm);
    input.push_back((y_coin - y) * y_norm);
  }
  input.push_back(x_vel * x_vel_norm);
  input.push_back(y_vel * y_vel_norm);
  if (!config.fourier_transforms.empty()) {
    size_t fourier_index = 0;
    std::vector<float> fourier;
    config.fourier_transforms[fourier_index++ % config.fourier_transforms.size()](x * x_norm,
                                                                                  fourier);
    for (const auto &f : fourier) {
      input.push_back(f);
    }
    config.fourier_transforms[fourier_index++ % config.fourier_transforms.size()](y * y_norm,
                                                                                  fourier);
    for (const auto &f : fourier) {
      input.push_back(f);
    }
    config.fourier_transforms[fourier_index++ % config.fourier_transforms.size()](x_coin * x_norm,
                                                                                  fourier);
    for (const auto &f : fourier) {
      input.push_back(f);
    }
    config.fourier_transforms[fourier_index++ % config.fourier_transforms.size()](y_coin * y_norm,
                                                                                  fourier);
    for (const auto &f : fourier) {
      input.push_back(f);
    }
  }
}

size_t ContinuousSeek::get_action_count() {
  return 2; // x and y acceleration
}

size_t ContinuousSeek::get_player_count() {
  return 1; // single player for now
}

std::string ContinuousSeek::get_name() {
  return "ContinuousSeek";
}

void ContinuousSeek::render(std::vector<uint32_t> &pixels) {
  // Ensure pixels is the right size
  pixels.resize(get_resolution().first * get_resolution().second);

  // Clear the screen (fill with black)
  std::fill(pixels.begin(), pixels.end(), rendering::make_color(0, 0, 0, 255));

  float scale = get_resolution().first / config.world_width;

  // Draw previous coin
  int pcoin_x = static_cast<int>((px_coin + config.world_width / 2) * get_resolution().first /
                                 config.world_width);
  int pcoin_y = static_cast<int>((py_coin + config.world_height / 2) * get_resolution().second /
                                 config.world_height);
  rendering::draw_circle(pixels, get_resolution(), pcoin_x, pcoin_y,
                         static_cast<int>(config.coin_radius * scale),
                         rendering::make_color(127, 127, 0, 255));

  // Draw player
  int player_x =
      static_cast<int>((x + config.world_width / 2) * get_resolution().first / config.world_width);
  int player_y = static_cast<int>((y + config.world_height / 2) * get_resolution().second /
                                  config.world_height);
  rendering::draw_circle(pixels, get_resolution(), player_x, player_y, scale,
                         rendering::make_color(255, 255, 255, 255));

  // Draw coin
  int coin_x = static_cast<int>((x_coin + config.world_width / 2) * get_resolution().first /
                                config.world_width);
  int coin_y = static_cast<int>((y_coin + config.world_height / 2) * get_resolution().second /
                                config.world_height);
  rendering::draw_circle(pixels, get_resolution(), coin_x, coin_y,
                         static_cast<int>(config.coin_radius * scale),
                         rendering::make_color(255, 255, 0, 255));

  // Draw a bar at the top corresponding to the player's progress towards the coin
  float dx = x_coin - px_coin;
  float dy = y_coin - py_coin;
  float coin_spawn_dist = std::sqrt(dx * dx + dy * dy);
  dx = x_coin - x;
  dy = y_coin - y;
  float player_coin_dist = std::sqrt(dx * dx + dy * dy);
  float progress_towards_coin = (coin_spawn_dist - player_coin_dist) / coin_spawn_dist;
  int bar_width = static_cast<int>(get_resolution().first * progress_towards_coin);
  int bar_height = 2; // Fixed height for the progress bar
  rendering::draw_rect(pixels, get_resolution(), 0, 0, bar_width, bar_height,
                       rendering::make_color(0, 255, 0, 255));
}

std::pair<int, int> ContinuousSeek::get_resolution() {
  // scale up arbitrarily
  return {static_cast<int>(config.world_width * 8), static_cast<int>(config.world_height * 8)};
}

std::unique_ptr<Game<obs::Simple>> ContinuousSeek::clone() const {
  return std::make_unique<ContinuousSeek>(*this);
}
