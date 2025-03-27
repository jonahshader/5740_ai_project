cmake_minimum_required(VERSION 3.28)
project(sim CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# # Eigen library
# FetchContent_Declare(
#   eigen
#   URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
#   # No hash for safety, as the exact URL might change
# )
# # We only need the headers, so we don't need to build Eigen's tests
# set(BUILD_TESTING OFF CACHE BOOL "Build Eigen tests" FORCE)
# FetchContent_MakeAvailable(eigen)

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

# Create main executable
add_executable(sim
  src/models/human.cpp
  src/models/human.h
  src/models/mlp_simple.cpp
  src/models/mlp_simple.h
  src/optimizers/simple.cpp
  src/optimizers/simple.h
  src/core_render.cpp
  src/core_render.h
  src/core.cpp
  src/core.h
  src/fixed_point.h
  src/interfaces.h
  src/main.cpp
  src/neural_net.h
  src/parse_map.h
  src/pixel_game.cpp
  src/pixel_game.h
  ${lodepng_SOURCE_DIR}/lodepng.cpp
)

# Link libraries
target_link_libraries(sim PRIVATE
  SDL2::SDL2
  OpenMP::OpenMP_CXX
)

# Include headers
target_include_directories(sim PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/models
  ${lodepng_SOURCE_DIR}
  # ${eigen_SOURCE_DIR}
)

# Platform-specific configurations
if(WIN32)
  target_link_libraries(sim PRIVATE SDL2::SDL2main)
  if(MSVC)
    # Add win_subsystem:windows to remove console window
    set_target_properties(sim PROPERTIES
      LINK_FLAGS "/SUBSYSTEM:WINDOWS"
      LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
      LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE"
    )

    # Use the MultiThreaded DLL runtime library for both Debug and Release
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
  endif()

  # Copy SDL2 DLL to output directory on Windows
  add_custom_command(TARGET sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL2::SDL2>
    $<TARGET_FILE_DIR:sim>
  )
endif()
