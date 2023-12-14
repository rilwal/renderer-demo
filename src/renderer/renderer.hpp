#pragma once

/*
TODO: Renderer
	TODO: Camera and projections

	TODO: Lighting
		TODO: Store light info and pass to shaders
		TODO: Shadow mapping? -> Soft shadows??

	TODO: HDR Pipeline and postprocessing
		TODO: Render to HDR texture
		TODO: Bloom
		TODO: Tonemapping
		TODO: Gamma correction
		TODO: Basic brightness, contrast etc.

	TODO: Model rendering
	
	TODO: Basic primitive rendering
		TODO: Cubes
		TODO: Spheres
		TODO: Planes

	TODO: 2D rendering functions
		TODO: Quads
		TODO: Text

	TODO: Batched rendering?
	TODO: Instanced rendering?
*/

// TODO: Kill this!
struct GLFWwindow;


#include <string>

#include <glm.hpp>


class Window {
public:
	Window(int width, int height, std::string title);
	~Window();

	glm::ivec2 get_size();

	operator GLFWwindow* () { return platform_window; }

private:
	GLFWwindow* platform_window;
};


class Renderer {
public:
	Renderer() : window(1920, 1080, "Engine") {};

	void initialize();
	void shutdown();

	bool should_close();

	void begin_frame();
	void end_frame();

	//TODO: Kill this when possible
	GLFWwindow* get_platform_window();

	int get_window_width();
	int get_window_height();

private:
	Window window;
};