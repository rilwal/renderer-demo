
#include "renderer.hpp"

#include "glad/gl.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuizmo.h"

#include "util.hpp"
#include "instrumentation/instrumentor.hpp"

const char* glsl_version = "#version 460";


void gl_post_call(void* ret, const char* name, GLADapiproc apiproc, int len_args, ...) {
	uint32_t error = glad_glGetError();
	if (error != GL_NO_ERROR) {
		fprintf(stderr, "GL Error in %s: (%u) %s\n", name, error, gl_error_name(error).c_str());
	}
}

// Initialize OpenGL and shit
void Renderer::initialize() {
	glfwInit();


	gladLoadGL(glfwGetProcAddress);
	gladSetGLPostCallback(gl_post_call);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_CULL_FACE);

	glCullFace(GL_BACK);

	// Next, let's configure IMGUI

	// TODO: Switch to docking branch!

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// TODO: Either bundle a font, or make this cross platform!
	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);

}

void Renderer::shutdown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	
	glfwTerminate();
}


bool Renderer::should_close() {
	return glfwWindowShouldClose(window) == 1;
}


void Renderer::begin_frame() {
	PROFILE_FUNC();

	glfwPollEvents();

	int32_t display_w, display_h;

	glfwGetFramebufferSize(window, &display_w, &display_h);
	//glViewport(0, 0, display_w, display_h);
	GL_ERROR_CHECK();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	//ImGui::ShowDemoWindow();
	GL_ERROR_CHECK();

}


void Renderer::end_frame() {
	PROFILE_FUNC();

	// imgui stuff
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	
	glfwSwapBuffers(window);
	GL_ERROR_CHECK();


}


GLFWwindow* Renderer::get_platform_window() {
	return window;
}


int Renderer::get_window_width() {
	int width;
	glfwGetWindowSize(window, &width, nullptr);
	return width;
}

int Renderer::get_window_height() {
	int height;
	glfwGetWindowSize(window, &height, nullptr);

	return height;
}

Window::Window(int width, int height, std::string title) {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	//glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	platform_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

	glfwSetWindowSizeCallback(platform_window, [](GLFWwindow* win, int w, int h) {
		glViewport(0, 0, w, h);
		glm::ivec2 window_size_i = glm::ivec2(w, h);
	});

	glfwMakeContextCurrent(platform_window);
	glfwSwapInterval(0);
}

Window::~Window() {
	glfwDestroyWindow(platform_window);
}

glm::ivec2 Window::get_size() {
	int w, h;
	glfwGetWindowSize(platform_window, &w, &h);
	return glm::ivec2(w, h);
}
