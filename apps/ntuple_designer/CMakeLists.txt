add_executable(ntuple_designer main.cpp)

target_link_libraries(ntuple_designer
  PRIVATE
    contrast_core
)

target_include_directories(ntuple_designer
  PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
)
