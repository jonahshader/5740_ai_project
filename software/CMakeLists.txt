cmake_minimum_required(VERSION 3.28)
project(sim CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable consistent runtime library propagation to dependencies
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)

# Set runtime library based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
else()
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# Include FetchContent for downloading dependencies
include(FetchContent)

# Try to find system SDL2 first
find_package(SDL2 QUIET)

if(NOT SDL2_FOUND)
  # If system SDL2 is not found, fetch it
  FetchContent_Declare(
    SDL2
    URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-2.30.3.tar.gz
    URL_HASH SHA256=820440072f8f5b50188c1dae104f2ad25984de268785be40c41a099a510f0aec
  )

  # Enable X11 support explicitly
  set(SDL_X11 ON CACHE BOOL "Enable X11 support" FORCE)
  set(SDL_X11_SHARED ON CACHE BOOL "Use shared X11 libraries" FORCE)

  FetchContent_MakeAvailable(SDL2)
endif()

# GLAD
FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/jonahshader/glad_opengl46.git
        GIT_TAG main
)
FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
    FetchContent_Populate(glad)
    add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Dear ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.2
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )
  target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
  )
  target_link_libraries(imgui PRIVATE SDL2::SDL2)
endif()

# SerialPort
FetchContent_Declare(
  serial_cpp
  GIT_REPOSITORY https://github.com/ami-iit/serial_cpp.git
  GIT_TAG        v1.3.3 # or use the tag or commit you prefer
)

FetchContent_MakeAvailable(serial_cpp)

# lodepng
FetchContent_Declare(
  lodepng
  GIT_REPOSITORY https://github.com/lvandeve/lodepng.git
  GIT_TAG master
)
FetchContent_GetProperties(lodepng)
if(NOT lodepng_POPULATED)
  FetchContent_Populate(lodepng)
endif()

# Find OpenMP
find_package(OpenMP REQUIRED)

# Define common source files (excluding main.cpp)
set(COMMON_SOURCES
  src/games/game.h
  src/games/jnb_render.cpp
  src/games/jnb_render.h
  src/games/jnb.cpp
  src/games/jnb.h
  src/models/human.cpp
  src/models/human.h
  src/models/mlp_map_lut.cpp
  src/models/mlp_map_lut.h
  src/models/mlp_simple.cpp
  src/models/mlp_simple.h
  src/models/model.h
  src/models/pl_nn_model.cpp
  src/models/pl_nn_model.h
  src/optimizers/simple.h
  src/comms.cpp
  src/comms.h
  src/fixed_point.h
  src/neural_net.h
  src/parse_map.h
  src/pixel_game.cpp
  src/pixel_game.h
  src/pl_nn.cpp
  src/pl_nn.h
  src/play.cpp
  src/play.h
  src/rendering.cpp
  src/rendering.h
  ${lodepng_SOURCE_DIR}/lodepng.cpp
)

# Define common include directories
set(COMMON_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/models
  ${CMAKE_CURRENT_SOURCE_DIR}/src/games
  ${lodepng_SOURCE_DIR}
  ${serialport_SOURCE_DIR}/include
  ${glad_SOURCE_DIR}/include
)

# Define common libraries
set(COMMON_LIBRARIES
  SDL2::SDL2
  glad
  OpenMP::OpenMP_CXX
  serial_cpp::serial_cpp
  imgui
)

# Create main executable
add_executable(sim
  ${COMMON_SOURCES}
  src/main.cpp
)
target_link_libraries(sim PRIVATE ${COMMON_LIBRARIES})
target_include_directories(sim PRIVATE ${COMMON_INCLUDE_DIRS})

# Platform-specific configurations
if(WIN32)
  target_link_libraries(sim PRIVATE SDL2::SDL2main)
  if(MSVC)
    set_target_properties(sim PROPERTIES
      LINK_FLAGS "/SUBSYSTEM:WINDOWS"
      LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
      LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE"
    )
  endif()
  add_custom_command(TARGET sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL2::SDL2>
    $<TARGET_FILE_DIR:sim>
  )
endif()

# Verilator-based executable (Linux only)
if(NOT WIN32)
  find_package(verilator HINTS $ENV{VERILATOR_ROOT} QUIET)
  if(verilator_FOUND)
    add_executable(verilog_sim
      ${COMMON_SOURCES}
      src/main_sim.cpp
    )
    verilate(verilog_sim SOURCES game_test.v)

    target_include_directories(verilog_sim PRIVATE
      ${COMMON_INCLUDE_DIRS}
      ${VERILATOR_ROOT}/include
    )

    # Link with common libraries
    target_link_libraries(verilog_sim PRIVATE ${COMMON_LIBRARIES})
    message(STATUS "Verilator found. Building verilog_sim target.")
  else()
    message(STATUS "Verilator not found. Skipping verilog_sim target.")
  endif()
endif()
