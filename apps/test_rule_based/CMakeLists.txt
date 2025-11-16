add_executable(test_rule_based main.cpp)

target_link_libraries(test_rule_based
  PRIVATE
    contrast_core
    contrast_engine
)

target_include_directories(test_rule_based
  PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/engine/include
)
