# Minimum CMake version we expect; older versions might miss commands we use.
cmake_minimum_required(VERSION 3.10)

# Project name + language; set CXX so we get C++ defaults (not plain C).
project(Orderbook_Simulation LANGUAGES CXX)

# Request C++17 and make it a hard requirement for all targets.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # avoid compiler-specific GNU extensions by default

# Default to a Release build if the user doesn't pick one (Debug/RelWithDebInfo/etc).
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Find Boost.System for the API server (Beast needs it) and pthreads for ASIO.
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# Common warning flags for all targets (tweak as you like).
set(COMMON_WARNINGS -Wall -Wextra -Wpedantic)

# CLI executable: interactive terminal app.
add_executable(orderbook_cli
  CLI_interface.cpp
  orderbook.cpp
)
target_compile_options(orderbook_cli PRIVATE ${COMMON_WARNINGS})

# API server executable: serves HTTP + UI.
add_executable(api_server
  api_server.cpp
  orderbook.cpp
)
target_compile_options(api_server PRIVATE ${COMMON_WARNINGS})
target_link_libraries(api_server PRIVATE Boost::system Threads::Threads)
