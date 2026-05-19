#pragma once

#include <vector>
#include <optional>
#include <array>
#include <glm/glm.hpp>

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

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription binding_description{};
		binding_description.binding = 0;
		binding_description.stride = sizeof(Vertex);
		binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return binding_description;
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};

		attribute_descriptions[0].binding = 0;
		attribute_descriptions[0].location = 0;
		attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions[0].offset = offsetof(Vertex, pos);

		attribute_descriptions[1].binding = 0;
		attribute_descriptions[1].location = 1;
		attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[1].offset = offsetof(Vertex, color);

		return attribute_descriptions;
	}
};

class Application {
public:
	Application();
	~Application();

	void Loop();

	bool m_framebuffer_resized = false;

private:

	const std::vector<Vertex> vertices = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	void DrawFrame();

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
	void RecreateSwapChain();

	void CleanupSwapChain();

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	void CreateLogicalDevice();

	void CreateGraphicsPipeline();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void CreateRenderPass();

	void CreateFramebuffers();

	bool CheckExtension(const char** required, uint32_t required_count);

	std::vector<const char*> GetRequiredExtensions();
	bool CheckValidationLayerSupport();

	void SetupDebugMessenger();
		
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);

	void CreateSurface();

	void CreateImageView();

	void CreateCommandPool();
	void CreateCommandBuffers();

	void CreateVertexBuffer();

	uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);

	void RecordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);

	void CreateSyncObjects();

	GLFWwindow* m_window;

	const int MAX_FRAMES_IN_FLIGHT = 2;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	uint32_t current_frame = 0;

	VkBuffer m_vertex_buffer;
	VkDeviceMemory m_vertex_buffer_memory;

	VkInstance m_instance;
	VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphics_queue;
	VkQueue m_present_queue;

	VkCommandPool m_command_pool;
	std::vector<VkCommandBuffer> m_command_buffers;

	VkSwapchainKHR m_swap_chain;
	std::vector<VkImage> m_swap_chain_images;
	VkFormat m_swap_chain_image_format;

	std::vector<VkFramebuffer> m_swap_chain_framebuffers;

	VkRenderPass m_render_pass;
	VkPipelineLayout m_pipeline_layout;
	VkPipeline m_graphics_pipeline;

	VkExtent2D m_swap_chain_extent;

	std::vector<VkImageView> m_swap_chain_image_views;

	VkDebugUtilsMessengerEXT m_debug_messenger;

	VkSurfaceKHR m_surface;

	std::vector<VkSemaphore> m_image_available_semaphores;
	std::vector<VkSemaphore> m_render_finish_semaphores;
	std::vector<VkFence> m_in_flight_fences;

	const std::vector<const char*> m_validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> m_device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	const std::vector<VkDynamicState> m_dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
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