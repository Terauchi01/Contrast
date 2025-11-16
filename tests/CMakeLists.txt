
include(FetchContent)
FetchContent_Declare(
	googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG main
)
FetchContent_MakeAvailable(googletest)

add_executable(test_game_state test_game_state.cpp)
target_link_libraries(test_game_state PRIVATE contrast_core gtest_main)
add_test(NAME test_game_state COMMAND test_game_state)

add_executable(test_rules test_rules.cpp)
target_link_libraries(test_rules PRIVATE contrast_core gtest_main)
add_test(NAME test_rules COMMAND test_rules)

add_executable(test_movegen test_movegen.cpp)
target_link_libraries(test_movegen PRIVATE contrast_core gtest_main)
add_test(NAME test_movegen COMMAND test_movegen)

add_executable(test_ai test_ai.cpp)
target_link_libraries(test_ai PRIVATE contrast_core contrast_engine gtest_main)
add_test(NAME test_ai COMMAND test_ai)

add_executable(test_ntuple test_ntuple.cpp)
target_link_libraries(test_ntuple PRIVATE contrast_core contrast_engine gtest_main)
add_test(NAME test_ntuple COMMAND test_ntuple)

add_executable(test_symmetry test_symmetry.cpp)
target_link_libraries(test_symmetry PRIVATE contrast_core gtest_main)
add_test(NAME test_symmetry COMMAND test_symmetry)
