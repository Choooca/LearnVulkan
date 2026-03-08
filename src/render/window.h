#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

class Window {

public:
	Window(int width, int height, std::string name);
	~Window();

	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;

	bool ShouldClose() { return glfwWindowShouldClose(window); };

private:
	
	void InitWindow();

	const int m_width;
	const int m_height;

	std::string m_name;

	GLFWwindow *window;
};