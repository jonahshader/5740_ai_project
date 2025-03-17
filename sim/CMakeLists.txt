cmake_minimum_required(VERSION 3.28)
project(sim CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent for downloading dependencies
include(FetchContent)

# SDL2
FetchContent_Declare(
  SDL2
  URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-2.30.3.tar.gz
  URL_HASH SHA256=820440072f8f5b50188c1dae104f2ad25984de268785be40c41a099a510f0aec
)
FetchContent_MakeAvailable(SDL2)

# Find OpenMP
find_package(OpenMP REQUIRED)

# Create main executable
add_executable(sim
  src/main.cpp
)

# Link libraries
target_link_libraries(sim PRIVATE
  SDL2::SDL2
  OpenMP::OpenMP_CXX
)

# Include headers
target_include_directories(sim PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
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