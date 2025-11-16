#include <iostream>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_layer.hpp"
#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"

// Declare renderer functions
void set_game_state(contrast::GameState* s);
void render_frame();

int main(int argc, char** argv) {
  // Get weights file path from command line or use default
  std::string weights_path = "ntuple_weights_vs_greedy.bin.100000";
  if (argc > 1) {
    weights_path = argv[1];
  }
  
  fprintf(stderr, "[gui_play] starting - Human vs AI\n");
  fprintf(stderr, "[gui_play] AI weights: %s\n", weights_path.c_str());
  
  if (!glfwInit()) {
    fprintf(stderr, "[gui_play] glfwInit failed\n");
    return -1;
  }
  
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
  GLFWwindow* window = glfwCreateWindow(1200, 800, "Contrast - Human vs AI", NULL, NULL);
  if (!window) {
    const char* errdesc = nullptr;
    int errcode = glfwGetError(&errdesc);
    fprintf(stderr, "[gui_play] glfwCreateWindow failed (code=%d desc=%s)\n", 
            errcode, errdesc ? errdesc : "(null)");

    // Try fallback
    fprintf(stderr, "[gui_play] attempting fallback with default window hints\n");
    glfwDefaultWindowHints();
    GLFWwindow* fb = glfwCreateWindow(1200, 800, "Contrast - Human vs AI (fallback)", NULL, NULL);
    if (!fb) {
      const char* desc2 = nullptr;
      int err2 = glfwGetError(&desc2);
      fprintf(stderr, "[gui_play] fallback glfwCreateWindow also failed (code=%d desc=%s)\n", 
              err2, desc2 ? desc2 : "(null)");
      glfwTerminate();
      return -1;
    }
    window = fb;
  }
  
  fprintf(stderr, "[gui_play] created window\n");
  glfwMakeContextCurrent(window);

  init_imgui(window);
  fprintf(stderr, "[gui_play] init_imgui done\n");

  contrast::GameState state;
  set_game_state(&state);
  
  // Set the weights path for the AI to use
  extern std::string g_weights_path;
  g_weights_path = weights_path;
  
  fprintf(stderr, "[gui_play] game state set; entering main loop\n");

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    new_frame();

    render_frame();

    ImGui::Render();
    int display_w, display_h; 
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    render_imgui();

    glfwSwapBuffers(window);
  }

  fprintf(stderr, "[gui_play] exiting main loop; shutting down\n");
  shutdown_imgui();
  glfwDestroyWindow(window);
  glfwTerminate();
  fprintf(stderr, "[gui_play] terminated\n");
  return 0;
}
