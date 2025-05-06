#include "jnb_render.h"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <fstream>

#include "lodepng.h"
#include "imgui.h"
#include "serial_cpp/serial.h"

#include "model.h"
#include "models/human.h"
#include "optimizers/simple.h"
#include "comms.h"
#include "game.h"

// constexpr int asdf = 0;

namespace jnb {

void imgui_training_config(GAConfig &ga_config, EvalConfig &eval_config) {
  ImGui::Begin("Training Config");

  // GA Configuration section
  ImGui::Text("Genetic Algorithm Configuration");

  // Mutation rate with slider
  ImGui::SliderFloat("Mutation Rate", &ga_config.mutation_rate, 0.0f, 1.0f, "%.2f");

  // Taper mutation rate checkbox
  ImGui::Checkbox("Taper Mutation Rate", &ga_config.taper_mutation_rate);

  // Max generations setting
  ImGui::InputInt("Max Generations", &ga_config.max_gen);
  if (ga_config.max_gen < 1)
    ga_config.max_gen = 1;

  // Run until stop checkbox
  ImGui::Checkbox("Run Until Stop", &ga_config.run_until_stop);

  // Tournament size
  ImGui::InputInt("Tournament Size", &ga_config.tournament_size);
  if (ga_config.tournament_size < 2)
    ga_config.tournament_size = 2;

  // Population size
  ImGui::InputInt("Population Size", &ga_config.population_size);
  if (ga_config.population_size < 2)
    ga_config.population_size = 2;

  // Model history settings
  ImGui::InputInt("Model History Size", &ga_config.model_history_size);
  // if (ga_config.model_history_size < 1)
  //   ga_config.model_history_size = 1;

  ImGui::InputInt("Model History Interval", &ga_config.model_history_interval);
  if (ga_config.model_history_interval < 1)
    ga_config.model_history_interval = 1;

  // Seed setting
  ImGui::InputScalar("Random Seed", ImGuiDataType_U64, &ga_config.seed);

  // Reference count
  ImGui::InputInt("Reference Count", &ga_config.reference_count);
  // if (ga_config.reference_count < 1)
  //   ga_config.reference_count = 1;

  // Evaluation interval
  ImGui::InputInt("Evaluation Interval", &ga_config.eval_interval);
  if (ga_config.eval_interval < 1)
    ga_config.eval_interval = 1;

  // Separator between GA config and Eval config
  ImGui::Separator();

  // Evaluation Configuration section
  ImGui::Text("Evaluation Configuration");

  // Seed count
  ImGui::InputInt("Seed Count", &eval_config.seed_count);
  if (eval_config.seed_count < 1)
    eval_config.seed_count = 1;

  // Frame limit
  ImGui::InputInt("Frame Limit", &eval_config.frame_limit);
  if (eval_config.frame_limit < 1)
    eval_config.frame_limit = 1;

  // Recycle seed
  ImGui::Checkbox("Recycle Seeds", &eval_config.recycle_seeds);

  // Add buttons for common actions
  if (ImGui::Button("Reset GA Config")) {
    ga_config = GAConfig{}; // Reset to default values
  }

  ImGui::SameLine();

  if (ImGui::Button("Reset Eval Config")) {
    eval_config = EvalConfig{}; // Reset to default values
  }

  ImGui::SameLine();

  if (ImGui::Button("Generate Random Seed")) {
    // Simple way to generate a random seed
    ga_config.seed = static_cast<uint64_t>(time(nullptr));
  }

  ImGui::End();
}

void imgui_state_control(PSPLState &pspl_state, set_uart_fun set_uart, const TileMap &map,
                         const GAConfig &ga_config, const EvalConfig &eval_config,
                         int &bram_to_save) {
  ImGui::Begin("State Control");
  switch (pspl_state) {
    case IDLE:
      ImGui::Text("Idle");
      if (ImGui::Button("Start Training")) {
        set_uart(TRAINING_GO_MSG);
        pspl_state = TRAINING;
      }
      if (ImGui::Button("Resume Training")) {
        set_uart(TRAINING_RESUME_MSG);
        pspl_state = TRAINING;
      }
      if (ImGui::Button("Watch AI vs AI")) {
        set_uart(PLAY_AGAINST_NN_FALSE);
        // set_uart(INFERENCE_GO_MSG);
      }
      if (ImGui::Button("Play against AI")) {
        set_uart(PLAY_AGAINST_NN_TRUE);
        // set_uart(INFERENCE_GO_MSG);
      }
      if (ImGui::Button("Send Map")) {
        send(map, set_uart);
      }
      if (ImGui::Button("Send Config")) {
        send(ga_config, eval_config, set_uart);
      }
      break;
    case TRAINING:
      ImGui::Text("Training");
      if (ImGui::Button("Pause Training")) {
        set_uart(TRAINING_STOP_MSG);
        // don't transition right away, since it takes time to pause/stop
      }
      // add slider for bram_to_save
      ImGui::SliderInt("BRAM to dump", &bram_to_save, 0, 143);
      if (ImGui::Button("Dump BRAM")) {
        set_uart(BRAM_DUMP_MSG);
      }
      break;
    case PLAYING:
      ImGui::Text("Playing");
      if (ImGui::Button("Stop Playing")) {
        set_uart(INFERENCE_STOP_MSG);
        pspl_state = IDLE;
      }
      break;
    default:
      break;
  }
  ImGui::End();
}

void imgui_plot_fitness(std::vector<float> &fitness_history) {
  ImGui::Begin("Fitness History");

  // Find min and max values for proper scaling
  float min_val = std::numeric_limits<float>::max();
  float max_val = std::numeric_limits<float>::lowest();
  for (const auto &val : fitness_history) {
    min_val = std::min(min_val, val);
    max_val = std::max(max_val, val);
  }

  // Add some padding to min/max to avoid exact edge cases
  float range = max_val - min_val;
  min_val -= range * 0.1f;
  max_val += range * 0.1f;

  // Plot with proper scaling
  ImGui::PlotLines("Fitness", fitness_history.data(), fitness_history.size(), 0, nullptr, min_val,
                   max_val, ImVec2(0, 80));

  if (ImGui::Button("Clear")) {
    fitness_history.clear();
  }

  ImGui::End();
}

bool connect_serial(std::shared_ptr<serial_cpp::Serial> &serial_connection, bool &is_connected,
                    const std::string &port) {
  try {
    // Close existing connection if any
    if (serial_connection && serial_connection->isOpen()) {
      serial_connection->close();
    }

    // Create new connection with fixed 115200 baud rate
    serial_connection =
        std::make_shared<serial_cpp::Serial>(port,
                                             115200, // Fixed baud rate
                                             serial_cpp::Timeout::simpleTimeout(25) // 25ms timeout
        );

    is_connected = serial_connection->isOpen();
    return is_connected;
  } catch (std::exception &e) {
    std::cerr << "Error connecting to serial port: " << e.what() << std::endl;
    is_connected = false;
    return false;
  }
}

void imgui_serial(std::shared_ptr<serial_cpp::Serial> &serial_connection,
                  std::vector<serial_cpp::PortInfo> &available_ports, bool &is_connected,
                  std::string &selected_port) {
  ImGui::Begin("Serial");

  if (!is_connected) {
    if (ImGui::Button("Refresh Ports")) {
      available_ports = serial_cpp::list_ports();
    }

    // Available ports dropdown
    if (ImGui::BeginCombo("Serial Port", selected_port.c_str())) {
      for (const auto &port_info : available_ports) {
        bool is_selected = (selected_port == port_info.port);
        // Display port with description
        std::string port_display = port_info.port + " - " + port_info.description;
        if (ImGui::Selectable(port_display.c_str(), is_selected)) {
          selected_port = port_info.port;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    // Connect button
    if (ImGui::Button("Connect") && !selected_port.empty()) {
      if (connect_serial(serial_connection, is_connected, selected_port)) {
        // Successfully connected
        std::cout << "Connected to " << selected_port << " at 115200 baud" << std::endl;
      } else {
        // Failed to connect
        std::cerr << "Failed to connect to " << selected_port << std::endl;
      }
    }
  } else {
    // Disconnect button
    if (ImGui::Button("Disconnect")) {
      if (serial_connection->isOpen()) {
        serial_connection->close();
      }
      is_connected = false;
    }
  }

  // Always call End()
  ImGui::End();
}

void run_on_pl(const std::string &map_filename) {
  // initialize game state
  // GameState state = jnb::init(map_filename, 1);
  JnBGame game(map_filename, -1);

  PixelGame window("JnB Sim", 640, 480, 60);

  PSPLState program_state{PSPLState::WAIT_FOR_UART_CONN};

  PlayerInput input;

  std::vector<uint8_t> spritesheet;
  uint32_t w, h;
  auto error = lodepng::decode(spritesheet, w, h, "tiles.png"); // TODO: move path to constexpr
  if (error) {
    std::cerr << "Error loading spritesheet: " << lodepng_error_text(error) << std::endl;
    throw std::runtime_error("Failed to load spritesheet");
  }
  std::cout << "Loaded tiles.png" << std::endl;
  std::cout << "Width: " << w << ", Height: " << h << std::endl;

  // TODO: these don't need to be shared ptrs
  auto ga_config = std::make_shared<GAConfig>();
  auto eval_config = std::make_shared<EvalConfig>();
  std::vector<float> fitness_history;
  int bram_to_save{0};

  // Serial connection variables
  std::shared_ptr<serial_cpp::Serial> serial_connection = nullptr;
  bool is_connected = false;
  std::string selected_port;
  std::vector<serial_cpp::PortInfo> available_ports;

  // Initial port refresh
  available_ports = serial_cpp::list_ports();
  std::cout << "Retrieved initially available serial ports." << std::endl;
  std::cout << "Available ports: " << available_ports.size() << std::endl;
  for (const auto &port_info : available_ports) {
    std::cout << port_info.port << " - " << port_info.description << std::endl;
  }

  set_uart_fun set_uart = [&](std::uint8_t byte) {
    std::uint8_t buffer[1];
    buffer[0] = byte;
    std::cout << "Writing " << static_cast<int>(byte) << " to serial" << std::endl;
    serial_connection->write(buffer, 1);
  };
  get_uart_blocking_fun get_uart_blocking = [&]() {
    std::uint8_t buffer[1];
    buffer[0] = 0;

    // Record start time
    auto start_time = std::chrono::steady_clock::now();

    // Set timeout duration to 25ms (matching your existing timeout)
    const auto timeout_duration = std::chrono::milliseconds(25);

    // Loop until we get data or timeout
    while (true) {
      // Check if data is available
      if (serial_connection) {
        auto available_before = serial_connection->available();
        if (serial_connection->available() > 0) {
          serial_connection->read(buffer, 1);
          auto available_after = serial_connection->available();
          // std::cout << "Available before: " << available_before
          //           << ", Available after: " << available_after << std::endl;
          return buffer[0];
        }
      }

      // Check if we've timed out
      auto current_time = std::chrono::steady_clock::now();
      if (current_time - start_time > timeout_duration) {
        // Timeout occurred
        std::cerr << "Error: Serial read timed out" << std::endl;
        throw std::runtime_error("Serial read timed out");
      }

      // Sleep for a brief period to avoid busy-waiting
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  };
  get_uart_non_blocking_fun get_uart_non_blocking = [&]() -> std::optional<std::uint8_t> {
    std::uint8_t buffer[1];
    buffer[0] = 0;
    if (serial_connection && serial_connection->available() > 0) {
      serial_connection->read(buffer, 1);
      std::cout << "Non-blocking received: " << static_cast<int>(buffer[0]) << std::endl;
      return buffer[0];
    } else {
      return std::nullopt;
    }
  };

  // combine all imgui lambdas
  auto combined_imgui_lambda = [&]() {
    // imgui_training_config(*ga_config, *eval_config);
    // if (program_state == WAIT_FOR_UART_CONN) {
    //   imgui_serial(serial_connection, available_ports, is_connected, selected_port);
    // }
    imgui_plot_fitness(fitness_history);
    switch (program_state) {
      case WAIT_FOR_UART_CONN:
        imgui_serial(serial_connection, available_ports, is_connected, selected_port);
        break;
      case IDLE:
        imgui_training_config(*ga_config, *eval_config);
        break;
      default:
        break;
    }

    if (program_state != WAIT_FOR_UART_CONN) {
      imgui_state_control(program_state, set_uart, game.state.map, *ga_config, *eval_config,
                          bram_to_save);
    }

    // state transitions
    if (!is_connected) {
      program_state = WAIT_FOR_UART_CONN;
    }
    switch (program_state) {
      case WAIT_FOR_UART_CONN:
        if (is_connected) {
          program_state = IDLE;
        }
        break;
      default:
        break;
    }
  };

  auto update_lambda = [&]() {
    if (program_state == PLAYING) {
      // send player input, receive game state
      send(input, set_uart);
    }

    while (serial_connection != nullptr && serial_connection->isOpen() &&
           serial_connection->available() > 0) {
      std::optional<msg_obj> msg_maybe = receive(get_uart_blocking, get_uart_non_blocking);
      auto overload = Overload{
          [&](GameState gs) {
            std::cout << "Got GameState" << std::endl;
            // transfer relevant state
            game.state.p1 = gs.p1;
            game.state.p2 = gs.p2;
            game.state.coin_pos = gs.coin_pos;
            game.state.age = gs.age;
          },
          [&](GAStatus ga_status) {
            std::cout << "Gen " << ga_status.current_gen
                      << " total ref. fit.: " << ga_status.reference_fitness << std::endl;
            fitness_history.emplace_back(static_cast<float>(ga_status.reference_fitness));
          },
          [&](std::vector<std::uint8_t> bram) {
            // write to a file called bram_{bram_to_save}_num{num}.dat
            static int num = 0;
            std::ofstream bram_file("bram_" + std::to_string(bram_to_save) + "_num" +
                                        std::to_string(num) + ".dat",
                                    std::ios::binary);
            if (bram_file.is_open()) {
              bram_file.write(reinterpret_cast<const char *>(bram.data()), bram.size());
              bram_file.close();
              std::cout << "Wrote bram_" << num << ".dat" << std::endl;
              num++;
            } else {
              std::cerr << "Error opening file for writing: bram_" << num << ".dat" << std::endl;
            }
          },
          [&](std::uint8_t byte) {
            switch (byte) {
              case NE_IS_IDLE:
                std::cout << "NE is idle" << std::endl;
                program_state = IDLE;
                break;
              case NE_IS_TRAINING:
                std::cout << "NE is training" << std::endl;
                program_state = TRAINING;
                break;
              case NE_IS_PLAYING:
                std::cout << "NE is playing" << std::endl;
                program_state = PLAYING;
                break;
              case TEST_RESPONSE_MSG:
                std::cout << "Received test response: " << static_cast<int>(byte) << std::endl;
                break;
              default:
                std::cerr << "Unknown message byte: " << byte << std::endl;
                break;
            }
          },
          [](auto) {
            // handle other types
            std::cerr << "Unknown message type" << std::endl;
          }};

      if (msg_maybe) {
        std::visit(overload, msg_maybe.value());
      } else {
        std::cerr << "Error: No message received, but serial_connection->available() > 0"
                  << std::endl;
      }
    }
  };
  // SDLK_LEFT, SDLK_RIGHT, SDLK_UP
  constexpr SDL_KeyCode LEFT = SDLK_LEFT;
  constexpr SDL_KeyCode RIGHT = SDLK_RIGHT;
  constexpr SDL_KeyCode JUMP = SDLK_UP;

  auto handle_input_lambda = [&](SDL_Event &event) {
    auto k = event.key.keysym.sym;
    if (event.type == SDL_KEYDOWN) {
      switch (k) {
        case LEFT:
          input.left = true;
          break;
        case RIGHT:
          input.right = true;
          break;
        case JUMP:
          input.jump = true;
          break;
        default:
          break;
      }
    } else if (event.type == SDL_KEYUP) {
      switch (k) {
        case LEFT:
          input.left = false;
          break;
        case RIGHT:
          input.right = false;
          break;
        case JUMP:
          input.jump = false;
          break;
        default:
          break;
      }
    }
  };
  std::cout << "Launching game..." << std::endl;
  window.run(
      update_lambda,
      [&game](std::vector<uint32_t> &pixels) {
        game.render(pixels);
        return game.get_resolution();
      },
      handle_input_lambda, combined_imgui_lambda);
}

} // namespace jnb
