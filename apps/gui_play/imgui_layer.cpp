#include "imgui_layer.hpp"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <cstdio>

void init_imgui(void* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(window), true);

	// Choose GLSL version depending on GL driver; fallback to #version 150
	const char* gl_version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	const char* glsl_version = "#version 150";
	if (gl_version && gl_version[0] == '2') {
		// likely OpenGL 2.1 context -> use GLSL 120
		glsl_version = "#version 120";
	}
	fprintf(stderr, "[imgui_layer] GL_VERSION=%s, using GLSL %s\n", gl_version ? gl_version : "(null)", glsl_version);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void shutdown_imgui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void new_frame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void render_imgui() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
