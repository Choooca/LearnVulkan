#include "window.h"

Window::Window(int width, int height, std::string name) : m_width(width), m_height(height), m_name(name)
{
	InitWindow();
}

Window::~Window()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Window::InitWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
}
