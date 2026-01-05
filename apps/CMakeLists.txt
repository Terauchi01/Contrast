add_subdirectory(cli_selfplay)
add_subdirectory(cli_play)

# GUI components depend on GLFW / X11. Disable by default on headless systems.
option(BUILD_GUI "Build GUI applications (requires X11/GLFW)" OFF)
if(BUILD_GUI)
	add_subdirectory(gui_debug)
	add_subdirectory(gui_play)
	add_subdirectory(ntuple_designer)
endif()

add_subdirectory(train_ntuple)
add_subdirectory(eval_ntuple)
add_subdirectory(test_rule_based)
add_subdirectory(test_rulebased)
add_subdirectory(debug_rulebased)
add_subdirectory(test_mcts)

# add_subdirectory(web_server)  # Temporarily disabled

