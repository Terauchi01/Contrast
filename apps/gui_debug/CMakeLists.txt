cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.90
)
FetchContent_MakeAvailable(imgui)

add_executable(gui_debug
  main.cpp
  imgui_layer.cpp
  renderer.cpp
)

target_include_directories(gui_debug PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Collect ImGui sources (core + backends)
set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_sources(gui_debug PRIVATE ${IMGUI_SOURCES})
target_include_directories(gui_debug PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

find_package(OpenGL REQUIRED)

target_link_libraries(gui_debug PRIVATE contrast_core contrast_engine glfw OpenGL::GL)
