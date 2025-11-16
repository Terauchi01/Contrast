cmake_minimum_required(VERSION 3.10)
project(contrast VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" ON)

add_subdirectory(core)
add_subdirectory(engine)
add_subdirectory(apps)
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
