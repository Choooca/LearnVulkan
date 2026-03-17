#pragma once

#include <vector>

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
	
	bool CheckExtension(const char** required, uint32_t required_count);

	std::vector<const char*> GetRequiredExtensions();
	bool CheckValidationLayerSupport();

	void SetupDebugMessenger();

	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);

	GLFWwindow* m_window;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	VkInstance m_instance;

	VkDebugUtilsMessengerEXT debug_messenger;

	const std::vector<const char*> m_validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	#ifdef NDEBUG
		const bool m_enable_validation_layers = false;
	#else
		const bool m_enable_validation_layers = true;
	#endif

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
};