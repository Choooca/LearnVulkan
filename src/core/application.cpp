#include "application.h"

void Application::Loop()
{
	while (!window.ShouldClose()) {
		glfwPollEvents();
	}
}
