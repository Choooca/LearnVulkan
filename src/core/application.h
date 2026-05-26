#pragma once

#include <vector>
#include <optional>
#include <array>
#include <glm/glm.hpp>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;
	std::optional<uint32_t> transfer_family;

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
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 tex_coords;

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription binding_description{};
		binding_description.binding = 0;
		binding_description.stride = sizeof(Vertex);
		binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return binding_description;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && tex_coords == other.tex_coords;
	}

	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};

		attribute_descriptions[0].binding = 0;
		attribute_descriptions[0].location = 0;
		attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[0].offset = offsetof(Vertex, pos);

		attribute_descriptions[1].binding = 0;
		attribute_descriptions[1].location = 1;
		attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[1].offset = offsetof(Vertex, color);

		attribute_descriptions[2].binding = 0;
		attribute_descriptions[2].location = 2;
		attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions[2].offset = offsetof(Vertex, tex_coords);

		return attribute_descriptions;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.tex_coords) << 1);
		}
	};
}

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

class Application {
public:
	Application();
	~Application();

	void Loop();

	bool m_framebuffer_resized = false;

private:

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

	void CreateDescriptorSetLayout();
	void CreateDescriptorPool();
	void CreateDescriptorSet();

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

	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flag);

	void CreateSwapChainImageViews();

	void CreateCommandPool();
	void CreateCommandBuffers();

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
	void CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();

	void CreateDepthResources();

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();
	bool HasStencilComponent(VkFormat format);

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags property, VkImage& image, VkDeviceMemory& image_memory);

	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);

	VkCommandBuffer BeginSingleTimeCommand(VkCommandPool command_pool);
	void EndSingleTimeCommand(VkCommandBuffer command_buffer, VkCommandPool command_pool, VkQueue queue);

	uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);

	void RecordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);
	void UpdateUniformBuffer(uint32_t current_image);

	void CreateSyncObjects();

	void LoadModel();

	GLFWwindow* m_window;

	const int MAX_FRAMES_IN_FLIGHT = 2;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	const std::string MODEL_PATH = "viking_room.obj";
	const std::string TEXTURE_PATH = "viking_room.png";

	uint32_t current_frame = 0;
	
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VkBuffer m_vertex_buffer;
	VkDeviceMemory m_vertex_buffer_memory;

	VkBuffer m_index_buffer;
	VkDeviceMemory m_index_buffer_memory;

	VkImage m_texture_image;
	VkDeviceMemory m_texture_image_memory;
	VkImageView m_texture_image_view;
	VkSampler m_texture_sampler;

	VkImage m_depth_image;
	VkDeviceMemory m_depth_image_memory;
	VkImageView m_depth_image_view;

	std::vector<VkBuffer> m_uniform_buffers;
	std::vector<VkDeviceMemory> m_uniform_buffers_memory;
	std::vector<void*> m_uniform_buffers_mapped;

	VkInstance m_instance;
	VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphics_queue;
	VkQueue m_present_queue;
	VkQueue m_transfer_queue;

	VkCommandPool m_command_pool;
	VkCommandPool m_command_pool_transfer;
	std::vector<VkCommandBuffer> m_command_buffers;

	VkSwapchainKHR m_swap_chain;
	std::vector<VkImage> m_swap_chain_images;
	VkFormat m_swap_chain_image_format;

	std::vector<VkFramebuffer> m_swap_chain_framebuffers;

	VkRenderPass m_render_pass;
	
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkDescriptorPool m_descriptor_pool;
	std::vector<VkDescriptorSet> m_descriptor_sets;

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