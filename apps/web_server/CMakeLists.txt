# Web Server for Contrast Game
cmake_minimum_required(VERSION 3.14)

# Download httplib
include(FetchContent)
FetchContent_Declare(
  httplib
  URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.14.3.tar.gz
  URL_HASH SHA256=6f6222842e2eb3e8c0d8c42ae97b4c03e6c73c9c1ea60a5fb67e8c89a6f18663
)
FetchContent_MakeAvailable(httplib)

# Web server executable
add_executable(contrast_server
  main.cpp
  game_session.cpp
)

target_include_directories(contrast_server PRIVATE
  ${httplib_SOURCE_DIR}
)

target_link_libraries(contrast_server
  contrast_core
  contrast_engine
)

# C++17 required
set_target_properties(contrast_server PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)
