#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Application {
public:
	Application();
	~Application();

	void Loop();

private:

	void InitWindow();
	void CreateInstance();
	void CheckExtension(const char** required, uint32_t required_count);

	GLFWwindow* m_window;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	VkInstance m_instance;
};