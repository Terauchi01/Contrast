#include <iostream>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_layer.hpp"
#include "renderer.hpp"
#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"

int main() {
  fprintf(stderr, "[gui_debug] starting\n");
  if (!glfwInit()) {
    fprintf(stderr, "[gui_debug] glfwInit failed\n");
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // required for macOS core profile
  GLFWwindow* window = glfwCreateWindow(1200, 800, "contrast gui_debug", NULL, NULL);
  if (!window) {
  const char* errdesc = nullptr;
  int errcode = glfwGetError(&errdesc);
  fprintf(stderr, "[gui_debug] glfwCreateWindow failed (code=%d desc=%s)\n", errcode, errdesc ? errdesc : "(null)");

    // Print some environment hints
    if (getenv("SSH_CONNECTION") || getenv("SSH_CLIENT") || getenv("SSH_TTY")) {
      fprintf(stderr, "[gui_debug] SSH detected in environment - GUI may not be available over SSH.\n");
    }

    // Try fallback: reset hints but keep macOS-compatible context settings
    fprintf(stderr, "[gui_debug] attempting fallback with core-profile hints\n");
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    GLFWwindow* fb = glfwCreateWindow(1200, 800, "contrast gui_debug (fallback)", NULL, NULL);
    if (!fb) {
  const char* desc2 = nullptr;
  int err2 = glfwGetError(&desc2);
  fprintf(stderr, "[gui_debug] fallback glfwCreateWindow also failed (code=%d desc=%s)\n", err2, desc2 ? desc2 : "(null)");
      glfwTerminate();
      return -1;
    }
    window = fb;
  }
  fprintf(stderr, "[gui_debug] created window\n");
  glfwMakeContextCurrent(window);

  init_imgui(window);
  fprintf(stderr, "[gui_debug] init_imgui done\n");

  contrast::GameState state;
  set_game_state(&state); // set GameState pointer for renderer
  fprintf(stderr, "[gui_debug] game state set; entering main loop\n");

  // Optional: environment variable to hold the GUI open for a given number of seconds
  int hold_seconds = 0;
  if (const char* hs = getenv("GUI_DEBUG_HOLD_SECONDS")) {
    hold_seconds = atoi(hs);
    fprintf(stderr, "[gui_debug] GUI_DEBUG_HOLD_SECONDS=%d\n", hold_seconds);
  }

  double start_time = 0.0;
  if (hold_seconds > 0) start_time = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    new_frame();

    render_frame();

    ImGui::Render();
    int display_w, display_h; glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0,0,display_w,display_h);
    glClearColor(0.45f,0.55f,0.60f,1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    render_imgui();

    glfwSwapBuffers(window);

    if (hold_seconds > 0) {
      double now = glfwGetTime();
      if (now - start_time >= hold_seconds) {
        fprintf(stderr, "[gui_debug] hold time elapsed, exiting main loop\n");
        break;
      }
    }
  }

  fprintf(stderr, "[gui_debug] exiting main loop; shutting down\n");
  shutdown_imgui();
  glfwDestroyWindow(window);
  glfwTerminate();
  fprintf(stderr, "[gui_debug] terminated\n");
  return 0;
}
