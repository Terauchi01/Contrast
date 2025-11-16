cmake_minimum_required(VERSION 3.10)

add_executable(debug_rulebased main.cpp)
target_link_libraries(debug_rulebased 
  PRIVATE 
    contrast_core
    contrast_engine
)
target_include_directories(debug_rulebased 
  PRIVATE 
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/engine/include
)
