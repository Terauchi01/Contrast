cmake_minimum_required(VERSION 3.10)

add_executable(test_rulebased main.cpp)
target_link_libraries(test_rulebased 
  PRIVATE 
    contrast_core
    contrast_engine
)
target_include_directories(test_rulebased 
  PRIVATE 
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/engine/include
)
