#include "application.h"
#include <filesystem>
#include <iostream>
#include <vector>

Application::Application()
{
	InitWindow();
	CreateInstance();
}

Application::~Application()
{
	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_window);

	glfwTerminate();
}

void Application::Loop()
{
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
	}
}

#pragma region Initialization

void Application::InitWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT,"Dragibus", nullptr, nullptr);
}

void Application::CreateInstance()
{
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Dragibus";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;

	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	create_info.enabledExtensionCount = glfw_extension_count;
	create_info.ppEnabledExtensionNames = glfw_extensions;

	create_info.enabledLayerCount = 0;

	CheckExtension(glfw_extensions, glfw_extension_count);

	if(vkCreateInstance(&create_info, nullptr, &m_instance) != VK_SUCCESS)
		throw std::runtime_error("failed to create vk instance");
}

void Application::CheckExtension(const char** required, uint32_t required_count)
{
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> extensions(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

	for (uint32_t i = 0; i < required_count; ++i) {
		bool found = false;
		for (const auto& extension : extensions) {
			if (strcmp(required[i], extension.extensionName) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			throw std::runtime_error(std::string(required[i]) + " extension is required but not available");
		}
	}
}

#pragma endregion
