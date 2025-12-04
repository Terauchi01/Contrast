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
