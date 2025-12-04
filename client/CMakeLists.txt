add_executable(contrast_client main.cpp)

find_package(Threads REQUIRED)

target_include_directories(contrast_client PRIVATE ${PROJECT_SOURCE_DIR})

target_link_libraries(contrast_client PRIVATE contrast_engine Threads::Threads)
