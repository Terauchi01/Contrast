add_executable(train_ntuple main.cpp)

target_link_libraries(train_ntuple
  PRIVATE
    contrast_core
    contrast_engine
)

target_include_directories(train_ntuple
  PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/engine/include
)

# Alternating training program
add_executable(train_alternating train_alternating.cpp)

target_link_libraries(train_alternating
  PRIVATE
    contrast_core
    contrast_engine
)

target_include_directories(train_alternating
  PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/engine/include
)

# Parallel training program
add_executable(train_parallel train_parallel.cpp)

target_link_libraries(train_parallel
  PRIVATE
    contrast_core
    contrast_engine
)

target_include_directories(train_parallel
  PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/engine/include
)

# Self-play training program
add_executable(train_selfplay train_selfplay.cpp)

target_link_libraries(train_selfplay
  PRIVATE
    contrast_core
    contrast_engine
)

target_include_directories(train_selfplay
  PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/engine/include
)
