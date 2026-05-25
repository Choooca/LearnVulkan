#include "application.h"
#include <filesystem>
#include <iostream>
#include <set>
#include <spdlog/spdlog.h>
#include <utils/build_macro.h>
#include <utils/files.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Application::Application()
{
	spdlog::set_level(spdlog::level::debug);

	InitWindow();
	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateDepthResources();
	CreateFramebuffers();
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateSyncObjects();
}

Application::~Application()
{
	for (size_t i = 0; i < m_render_finish_semaphores.size(); ++i) {
		vkDestroySemaphore(m_device, m_render_finish_semaphores[i], nullptr);
	}

	CleanupSwapChain();

	vkDestroySampler(m_device, m_texture_sampler, nullptr);
	vkDestroyImageView(m_device, m_texture_image_view, nullptr);

	vkDestroyImage(m_device, m_texture_image, nullptr);
	vkFreeMemory(m_device, m_texture_image_memory, nullptr);

	for (size_t i = 0; i < m_uniform_buffers.size(); i++) {
		vkDestroyBuffer(m_device, m_uniform_buffers[i], nullptr);
		vkFreeMemory(m_device, m_uniform_buffers_memory[i], nullptr);
	}

	vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, nullptr);
	vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);

	vkDestroyBuffer(m_device, m_vertex_buffer, nullptr);
	vkFreeMemory(m_device, m_vertex_buffer_memory, nullptr);

	vkDestroyBuffer(m_device, m_index_buffer, nullptr);
	vkFreeMemory(m_device, m_index_buffer_memory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
		vkDestroyFence(m_device, m_in_flight_fences[i], nullptr);
	}
	vkDestroyCommandPool(m_device, m_command_pool, nullptr);
	vkDestroyCommandPool(m_device, m_command_pool_transfer, nullptr);

	vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
	vkDestroyRenderPass(m_device, m_render_pass, nullptr);

	vkDestroyDevice(m_device, nullptr);

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

	if (m_enable_validation_layers) {
		DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
	}

	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_window);

	glfwTerminate();
}

void Application::Loop()
{
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		DrawFrame();
	}

	vkDeviceWaitIdle(m_device);
}

#pragma region Debug

VKAPI_ATTR VkBool32 VKAPI_CALL Application::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		spdlog::error("validation layer: {}", pCallbackData->pMessage);
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		spdlog::warn("validation layer: {}", pCallbackData->pMessage);
	else
		spdlog::debug("validation layer: {}", pCallbackData->pMessage);

	return VK_FALSE;
}

VkResult Application::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Application::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void Application::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	create_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	create_info.pfnUserCallback = DebugCallback;
	create_info.pUserData = nullptr;
}

void Application::CreateSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface");
	}
}

VkImageView Application::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flag)
{
	VkImageViewCreateInfo view_info{};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flag;
	view_info.subresourceRange.layerCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseMipLevel = 0;

	VkImageView image_view;
	if (vkCreateImageView(m_device, &view_info, nullptr, &image_view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture_image_view");
	}

	return image_view;
}

void Application::CreateSwapChainImageViews()
{
	m_swap_chain_image_views.resize(m_swap_chain_images.size());

	for (int i = 0; i < m_swap_chain_images.size(); ++i) {
		m_swap_chain_image_views[i] = CreateImageView(m_swap_chain_images[i], m_swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void Application::CreateCommandPool()
{
	QueueFamilyIndices queue_family_indices = FindQueueFamilies(m_physical_device);
	
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

	if(vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");

	pool_info.queueFamilyIndex = queue_family_indices.transfer_family.value();
	pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	
	if (vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool_transfer) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool");
}

void Application::CreateCommandBuffers()
{
	m_command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = m_command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = (uint32_t)m_command_buffers.size();

	if(vkAllocateCommandBuffers(m_device, &alloc_info, m_command_buffers.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffers");	
}

void Application::CreateVertexBuffer()
{
	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	void* data;
	vkMapMemory(m_device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, vertices.data(), (size_t)buffer_size);
	vkUnmapMemory(m_device, staging_buffer_memory);

	CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertex_buffer, m_vertex_buffer_memory);

	CopyBuffer(staging_buffer, m_vertex_buffer, buffer_size);

	vkDestroyBuffer(m_device, staging_buffer, nullptr);
	vkFreeMemory(m_device, staging_buffer_memory, nullptr);
}

void Application::CreateIndexBuffer()
{
	VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

	VkBuffer  stating_buffer;
	VkDeviceMemory staging_buffer_memory;
	CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stating_buffer, staging_buffer_memory);

	void* data;
	vkMapMemory(m_device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, indices.data(), (size_t)buffer_size);
	vkUnmapMemory(m_device, staging_buffer_memory);

	CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_index_buffer, m_index_buffer_memory);

	CopyBuffer(stating_buffer, m_index_buffer, buffer_size);

	vkDestroyBuffer(m_device, stating_buffer, nullptr);
	vkFreeMemory(m_device, staging_buffer_memory, nullptr);
}

void Application::CreateUniformBuffers()
{
	VkDeviceSize buffer_size = sizeof(UniformBufferObject);

	m_uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		CreateBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniform_buffers[i], m_uniform_buffers_memory[i]);

		vkMapMemory(m_device, m_uniform_buffers_memory[i], 0, buffer_size, 0, &m_uniform_buffers_mapped[i]);
	}
}

void Application::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory)
{
	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;

	QueueFamilyIndices family_indices = FindQueueFamilies(m_physical_device);

	uint32_t shared_queue_family_indices[] = { family_indices.graphics_family.value(), family_indices.transfer_family.value() };

	if (family_indices.graphics_family != family_indices.transfer_family) {
		buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
		buffer_info.queueFamilyIndexCount = 2;
		buffer_info.pQueueFamilyIndices = shared_queue_family_indices;
	}
	else {
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateBuffer(m_device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer");
	}

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(m_device, buffer, &mem_requirements);

	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory");
	}

	vkBindBufferMemory(m_device, buffer, buffer_memory, 0);
}

void Application::CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
	VkCommandBuffer transfer_command_buffer = BeginSingleTimeCommand(m_command_pool_transfer);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = size;
	vkCmdCopyBuffer(transfer_command_buffer, src_buffer, dst_buffer, 1, &copy_region);
	
	EndSingleTimeCommand(transfer_command_buffer, m_command_pool_transfer, m_transfer_queue);
}

void Application::CreateTextureImage()
{
	int texture_width, texture_height, texture_channels;

	std::string str_path = std::string(TEXTURES_DIR) + "texture.jpg";
	const char* path = str_path.c_str();
	stbi_uc* pixels = stbi_load(path, &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
	VkDeviceSize image_size = texture_width * texture_height * 4;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	CreateBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	void* data;
	vkMapMemory(m_device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(image_size));
	vkUnmapMemory(m_device, staging_buffer_memory);

	stbi_image_free(pixels);

	CreateImage(texture_width, texture_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texture_image, m_texture_image_memory);
	TransitionImageLayout(m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyBufferToImage(staging_buffer, m_texture_image, static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height));
	TransitionImageLayout(m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(m_device, staging_buffer, nullptr);
	vkFreeMemory(m_device, staging_buffer_memory, nullptr);
}

void Application::CreateTextureImageView()
{
	m_texture_image_view = CreateImageView(m_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Application::CreateTextureSampler()
{
	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(m_physical_device, &properties);

	sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	if (vkCreateSampler(m_device, &sampler_info, nullptr, &m_texture_sampler)) {
		throw std::runtime_error("failed to create sampler");
	}
}

void Application::CreateDepthResources()
{
	VkFormat depth_format = FindDepthFormat();

	CreateImage(m_swap_chain_extent.width, m_swap_chain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depth_image, m_depth_image_memory);
	m_depth_image_view = CreateImageView(m_depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);

	TransitionImageLayout(m_depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkFormat Application::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format");
}

VkFormat Application::FindDepthFormat()
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool Application::HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Application::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags property, VkImage& image, VkDeviceMemory& image_memory)
{
	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.format = format;
	image_info.tiling = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	if (vkCreateImage(m_device, &image_info, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image");
	}

	VkMemoryRequirements image_memory_requirement;
	vkGetImageMemoryRequirements(m_device, image, &image_memory_requirement);

	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = image_memory_requirement.size;
	alloc_info.memoryTypeIndex = FindMemoryType(image_memory_requirement.memoryTypeBits, property);

	if (vkAllocateMemory(m_device, &alloc_info, nullptr, &image_memory)) {
		throw std::runtime_error("failed to allocate memory for an image");
	}

	vkBindImageMemory(m_device, image, image_memory, 0);
}

void Application::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer command_buffer = BeginSingleTimeCommand(m_command_pool_transfer);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.mipLevel = 0;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommand(command_buffer, m_command_pool_transfer, m_transfer_queue);
}

void Application::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkCommandBuffer command_buffer = BeginSingleTimeCommand(m_command_pool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (HasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}


	VkPipelineStageFlags source_stage{};
	VkPipelineStageFlags destination_stage{};

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		
		destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition");
	}

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	EndSingleTimeCommand(command_buffer, m_command_pool, m_graphics_queue);
}

VkCommandBuffer Application::BeginSingleTimeCommand(VkCommandPool command_pool)
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(m_device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void Application::EndSingleTimeCommand(VkCommandBuffer command_buffer, VkCommandPool command_pool, VkQueue queue)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;
	
	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(m_device, command_pool, 1, &command_buffer);
}

uint32_t Application::FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_properties);

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
		if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	throw std::runtime_error("failed to find suitable memory type");

	return 0;
}

void Application::RecordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	if(vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer");
	}

	VkRenderPassBeginInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = m_render_pass;
	render_pass_info.framebuffer = m_swap_chain_framebuffers[image_index];
	render_pass_info.renderArea.offset = { 0, 0 };
	render_pass_info.renderArea.extent = m_swap_chain_extent;

	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clear_values[1].depthStencil = { 1.0f, 0 };
	render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
	render_pass_info.pClearValues = clear_values.data();

	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

	VkBuffer vertex_buffers[] = { m_vertex_buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(command_buffer, m_index_buffer, 0, VK_INDEX_TYPE_UINT16);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swap_chain_extent.width);
	viewport.height = static_cast<float>(m_swap_chain_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swap_chain_extent;
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_sets[current_frame], 0, nullptr);
	vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
	vkCmdEndRenderPass(command_buffer);

	if(vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to end command buffer");
	}
}

void Application::UpdateUniformBuffer(uint32_t current_image)
{
	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), m_swap_chain_extent.width / (float)m_swap_chain_extent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	memcpy(m_uniform_buffers_mapped[current_image], &ubo, sizeof(UniformBufferObject));
}

void Application::CreateSyncObjects()
{
	m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_render_finish_semaphores.resize(m_swap_chain_images.size());
	m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_image_available_semaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_device, &fence_info, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame");
		}
	}

	for (size_t i = 0; i < m_swap_chain_images.size(); i++) {
		if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_render_finish_semaphores[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame");
		}
	}
}

#pragma endregion

#pragma region Initialization

void FrameBufferResizedCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	app->m_framebuffer_resized = true;
}

void Application::DrawFrame()
{
	vkWaitForFences(m_device, 1, &m_in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(m_device, m_swap_chain, UINT64_MAX, m_image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapChain();
		return;
	}
	else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
		throw std::runtime_error("failed to acquire swap chain image");
	}

	vkResetFences(m_device, 1, &m_in_flight_fences[current_frame]);

	vkResetCommandBuffer(m_command_buffers[current_frame], 0);
	RecordCommandBuffer(m_command_buffers[current_frame], image_index);

	UpdateUniformBuffer(current_frame);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { m_image_available_semaphores[current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffers[current_frame];

	VkSemaphore signal_semaphore = m_render_finish_semaphores[image_index];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &signal_semaphore;

	if(vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_in_flight_fences[current_frame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer");
	}

	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &signal_semaphore;

	VkSwapchainKHR swap_chains[] = { m_swap_chain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pImageIndices = &image_index;

	present_info.pResults = nullptr;

	result = vkQueuePresentKHR(m_present_queue, &present_info);

	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized){
		m_framebuffer_resized = false;
		RecreateSwapChain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image");
	}

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::InitWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT,"Dragibus", nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, FrameBufferResizedCallback);
}

void Application::CreateInstance()
{
	if (m_enable_validation_layers && !CheckValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested but not available");
	}

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

	if (m_enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
		create_info.ppEnabledLayerNames = m_validation_layers.data();	
	}
	else {
		create_info.enabledLayerCount = 0;
	}

	auto extensions = GetRequiredExtensions();

	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	if (!CheckExtension(extensions.data(), extensions.size())) {
		throw std::runtime_error("All extensions required are not available");
	}

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
	if (m_enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
		create_info.ppEnabledLayerNames = m_validation_layers.data();

		PopulateDebugMessengerCreateInfo(debug_create_info);
		create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
	} else {
		create_info.enabledLayerCount = 0;
		create_info.pNext = nullptr;
	}

	if(vkCreateInstance(&create_info, nullptr, &m_instance) != VK_SUCCESS)
		throw std::runtime_error("failed to create vk instance");
}

void Application::PickPhysicalDevice()
{
	uint32_t device_count;
	vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

	if(device_count == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support");

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

	for (const auto& device : devices) {
		if (IsDeviceSuitable(device)) {
			m_physical_device = device;
			break;
		}
	}

	if(m_physical_device == VK_NULL_HANDLE)
		throw std::runtime_error("failed to find suitable GPU");
}

bool Application::IsDeviceSuitable(VkPhysicalDevice physical_device)
{
	QueueFamilyIndices indices = FindQueueFamilies(physical_device);

	bool extension_supported = CheckDeviceExtensionSupport(physical_device);

	bool swap_chain_adequate = false;
	if (extension_supported) {
		SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(physical_device);
		swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
	}

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(physical_device, &supported_features);

	return indices.IsComplete() && extension_supported && swap_chain_adequate && supported_features.samplerAnisotropy;
}

bool Application::CheckDeviceExtensionSupport(VkPhysicalDevice physical_device)
{
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(m_device_extensions.begin(), m_device_extensions.end());

	for (const auto& extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}

SwapChainSupportDetails Application::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);

	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count; 
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);

	if (present_mode_count != 0) {
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, details.present_modes.data());
	}

	return details;
}

VkSurfaceFormatKHR Application::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	for (const auto& available_format : available_formats) {
		if(available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return available_format;
	}
	return available_formats[0];
}

VkPresentModeKHR Application::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
	for (const auto& available_present_mode : available_present_modes) {
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return available_present_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actual_extent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actual_extent;
	}
}

void Application::CreateSwapChain()
{
	SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(m_physical_device);

	VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
	VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes);
	VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities);

	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = m_surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(m_physical_device);
	uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

	if (indices.graphics_family != indices.present_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;

	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swap_chain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain");
	}

	vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, nullptr);
	m_swap_chain_images.resize(image_count);
	vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, m_swap_chain_images.data());

	m_swap_chain_image_format = surface_format.format;
	m_swap_chain_extent = extent;
}

void Application::RecreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_device);

	CleanupSwapChain();

	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateDepthResources();
	CreateFramebuffers();
}

void Application::CleanupSwapChain()
{
	vkDestroyImageView(m_device, m_depth_image_view, nullptr);
	vkDestroyImage(m_device, m_depth_image, nullptr);
	vkFreeMemory(m_device, m_depth_image_memory, nullptr);

	for (auto framebuffer : m_swap_chain_framebuffers) {
		vkDestroyFramebuffer(m_device, framebuffer, nullptr);
	}

	for (auto& image_view : m_swap_chain_image_views) {
		vkDestroyImageView(m_device, image_view, nullptr);
	}

	vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
}

QueueFamilyIndices Application::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto& queue_family : queue_families) {
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
		}

		if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			indices.transfer_family = i;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present_support);

		if (present_support) {
			indices.present_family = i;
		}

		i++;
	}

	if (!indices.transfer_family.has_value()) {
		indices.transfer_family = indices.graphics_family;
	}

	return indices;
}

void Application::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(m_physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {
		indices.graphics_family.value(),
		indices.present_family.value(),
		indices.transfer_family.value()
		};

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}
		
	VkPhysicalDeviceFeatures device_features{};
	device_features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();

	create_info.enabledExtensionCount = static_cast<uint32_t>(m_device_extensions.size());
	create_info.ppEnabledExtensionNames = m_device_extensions.data();

	create_info.pEnabledFeatures = &device_features;

	if (m_enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
		create_info.ppEnabledLayerNames = m_validation_layers.data();
	}
	else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_physical_device, &create_info, nullptr, &m_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device");
	}

	vkGetDeviceQueue(m_device, indices.graphics_family.value(), 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device, indices.present_family.value(), 0, &m_present_queue);
	vkGetDeviceQueue(m_device, indices.transfer_family.value(), 0, &m_transfer_queue);
}

void Application::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding ubo_layout_binding{};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding sampler_layout_binding{};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.pImmutableSamplers = nullptr;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };
	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings = bindings.data();

	if(vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_descriptor_set_layout) != VK_SUCCESS){
		throw std::runtime_error("failed to create descriptor set layout");
	}
}

void Application::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2>  pool_sizes{};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_descriptor_pool)) {
		throw std::runtime_error("failed to create descriptor pool");
	}
}

void Application::CreateDescriptorSet()
{
	std::vector<VkDescriptorSetLayout> descriptor_set_layout(MAX_FRAMES_IN_FLIGHT, m_descriptor_set_layout);
	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = m_descriptor_pool;
	alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	alloc_info.pSetLayouts = descriptor_set_layout.data();

	m_descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(m_device, &alloc_info, m_descriptor_sets.data())) {
		throw std::runtime_error("failed to allocate descriptor sets");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = m_uniform_buffers[i];
		buffer_info.offset = 0;
		buffer_info.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = m_texture_image_view;
		image_info.sampler = m_texture_sampler;

		std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = m_descriptor_sets[i];
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].pBufferInfo = &buffer_info;

		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].dstSet = m_descriptor_sets[i];
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].dstArrayElement = 0;
		descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[1].descriptorCount = 1;
		descriptor_writes[1].pImageInfo = &image_info;

		vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
	}
}

void Application::CreateGraphicsPipeline()
{
	auto vert_shader_code = ReadFile(std::string(SHADERS_DIR) + "simple_shader_vert.spv");
	auto frag_shader_code = ReadFile(std::string(SHADERS_DIR) + "simple_shader_frag.spv");

	VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);
	VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(m_dynamic_states.size());
	dynamic_state_create_info.pDynamicStates = m_dynamic_states.data();

	auto binding_description = Vertex::GetBindingDescription();
	auto attribute_descriptions = Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info{};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
	input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swap_chain_extent.width);
	viewport.height = static_cast<float>(m_swap_chain_extent.height);
	viewport.maxDepth = 1.0f;
	viewport.minDepth = 0.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = m_swap_chain_extent;

	VkPipelineViewportStateCreateInfo viewport_state_create_info{};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = &viewport;
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterization_create_info{};
	rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_create_info.depthClampEnable = VK_FALSE;
	rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_create_info.lineWidth = 1.0f;
	rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_create_info.depthBiasEnable = VK_FALSE;
	rasterization_create_info.depthBiasConstantFactor = 0.0f;
	rasterization_create_info.depthBiasClamp = 0.0f;
	rasterization_create_info.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling_create_info{};
	multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_create_info.sampleShadingEnable = VK_FALSE;
	multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling_create_info.minSampleShading = 1.0f;
	multisampling_create_info.pSampleMask = nullptr;
	multisampling_create_info.alphaToCoverageEnable = VK_FALSE;
	multisampling_create_info.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachement{};
	color_blend_attachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachement.blendEnable = VK_FALSE;
	color_blend_attachement.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachement.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachement.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachement.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending_create_info{};
	color_blending_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending_create_info.logicOpEnable = VK_FALSE;
	color_blending_create_info.logicOp = VK_LOGIC_OP_COPY;
	color_blending_create_info.attachmentCount = 1;
	color_blending_create_info.pAttachments = &color_blend_attachement;
	color_blending_create_info.blendConstants[0] = 0.0f;
	color_blending_create_info.blendConstants[1] = 0.0f;
	color_blending_create_info.blendConstants[2] = 0.0f;
	color_blending_create_info.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
	depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_info.depthTestEnable = VK_TRUE;
	depth_stencil_info.depthWriteEnable = VK_TRUE;
	depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_info.stencilTestEnable = VK_FALSE;

	VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 1;
	pipeline_layout_create_info.pSetLayouts = &m_descriptor_set_layout;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_device, &pipeline_layout_create_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipeline_create_info{};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount = 2;
	pipeline_create_info.pStages = shader_stages;
	pipeline_create_info.pVertexInputState = &vertex_input_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
	pipeline_create_info.pViewportState = &viewport_state_create_info;
	pipeline_create_info.pRasterizationState = &rasterization_create_info;
	pipeline_create_info.pMultisampleState = &multisampling_create_info;
	pipeline_create_info.pDepthStencilState = &depth_stencil_info;
	pipeline_create_info.pColorBlendState = &color_blending_create_info;
	pipeline_create_info.pDynamicState = &dynamic_state_create_info;
	pipeline_create_info.layout = m_pipeline_layout;
	pipeline_create_info.renderPass = m_render_pass;
	pipeline_create_info.subpass = 0;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &m_graphics_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline");
	}

	vkDestroyShaderModule(m_device, vert_shader_module, nullptr);
	vkDestroyShaderModule(m_device, frag_shader_module, nullptr);

}

VkShaderModule Application::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module;
	if (vkCreateShaderModule(m_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module");
	}

	return shader_module;
}

void Application::CreateRenderPass()
{
	VkAttachmentDescription depth_attachment{};

	depth_attachment.format = FindDepthFormat();
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription color_attachment{};
	color_attachment.format = m_swap_chain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference depth_attachment_ref{};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref{};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
	VkRenderPassCreateInfo render_pass_create_info{};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	render_pass_create_info.pAttachments = attachments.data();
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass;
	render_pass_create_info.dependencyCount = 1;
	render_pass_create_info.pDependencies = &dependency;

	if (vkCreateRenderPass(m_device, &render_pass_create_info, nullptr, &m_render_pass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}

void Application::CreateFramebuffers()
{
	m_swap_chain_framebuffers.resize(m_swap_chain_image_views.size());

	for (int i = 0; i < m_swap_chain_image_views.size(); ++i) {
		std::array<VkImageView, 2> attachments = {
			m_swap_chain_image_views[i],
			m_depth_image_view
		};	

		VkFramebufferCreateInfo framebuffer_create_info{};
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = m_render_pass;
		framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_create_info.pAttachments = attachments.data();
		framebuffer_create_info.width = m_swap_chain_extent.width;
		framebuffer_create_info.height = m_swap_chain_extent.height;
		framebuffer_create_info.layers = 1;

		if (vkCreateFramebuffer(m_device, &framebuffer_create_info, nullptr, &m_swap_chain_framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer");
		}
	}
}

bool Application::CheckExtension(const char** required, uint32_t required_count)
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
			return false;
		}
	}

	return true;
}

std::vector<const char*> Application::GetRequiredExtensions()
{
	uint32_t glfw_extension_count = 0;
	const char** glfw_extension = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extension, glfw_extension + glfw_extension_count);

	if (m_enable_validation_layers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool Application::CheckValidationLayerSupport()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* layer_name : m_validation_layers) {
		bool layer_found = false;

		for (const VkLayerProperties& available_layer : available_layers) {
			if (strcmp(available_layer.layerName, layer_name) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found) {
			return false;
		}
	}

	return true;
}

void Application::SetupDebugMessenger()
{
	if(!m_enable_validation_layers) return;

	VkDebugUtilsMessengerCreateInfoEXT create_info;
	PopulateDebugMessengerCreateInfo(create_info);

	if (CreateDebugUtilsMessengerEXT(m_instance, &create_info, nullptr, &m_debug_messenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to setup debug messenger");
	}
}

#pragma endregion