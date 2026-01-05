add_library(contrast_engine
  src/random_policy.cpp
  src/greedy_policy.cpp
  src/rule_based_policy.cpp
  src/rule_based_policy2.cpp
  src/mcts.cpp
  src/ntuple.cpp
)

target_include_directories(contrast_engine
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(contrast_engine PUBLIC contrast_core)

# Big pattern variant using ntuple_big.cpp
add_library(contrast_engine_big
  src/random_policy.cpp
  src/greedy_policy.cpp
  src/rule_based_policy.cpp
  src/rule_based_policy2.cpp
  src/mcts.cpp
  src/alphabeta.cpp
  src/ntuple_big.cpp
)

target_include_directories(contrast_engine_big
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(contrast_engine_big PUBLIC contrast_core)
