#pragma once

#include <vector>
#include <optional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool IsComplete() {
		return graphics_family.has_value() && present_family.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

class Application {
public:
	Application();
	~Application();

	void Loop();

private:

	void InitWindow();
	void CreateInstance();
	
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_format);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_present_modes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
	
	void CreateSwapChain();

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	void CreateLogicalDevice();

	bool CheckExtension(const char** required, uint32_t required_count);

	std::vector<const char*> GetRequiredExtensions();
	bool CheckValidationLayerSupport();

	void SetupDebugMessenger();
		
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);

	void CreateSurface();

	GLFWwindow* m_window;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	VkInstance m_instance;
	VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphics_queue;
	VkQueue m_present_queue;

	VkSwapchainKHR m_swap_chain;
	std::vector<VkImage> m_swap_chain_images;
	VkFormat m_swap_chain_image_format;
	VkExtent2D m_swap_chain_extent;

	VkDebugUtilsMessengerEXT m_debug_messenger;

	VkSurfaceKHR m_surface;

	const std::vector<const char*> m_validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> m_device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
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