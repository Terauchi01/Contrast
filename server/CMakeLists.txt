add_executable(contrast_server main.cpp)

find_package(Threads REQUIRED)

target_include_directories(contrast_server PRIVATE ${PROJECT_SOURCE_DIR})

target_link_libraries(contrast_server PRIVATE contrast_core Threads::Threads)
