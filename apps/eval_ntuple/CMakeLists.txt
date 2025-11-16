add_executable(eval_ntuple main.cpp)

target_link_libraries(eval_ntuple PRIVATE 
  contrast_core
  contrast_engine
)

target_include_directories(eval_ntuple PRIVATE 
  ${CMAKE_SOURCE_DIR}/core/include
  ${CMAKE_SOURCE_DIR}/engine/include
)
