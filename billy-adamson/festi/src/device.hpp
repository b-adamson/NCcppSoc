#pragma once

#include "window.hpp"

// lib
#include <glm/glm.hpp>

// std
#include <string>
#include <memory>
#include <vector>

namespace festi {

constexpr uint32_t FS_UNSPECIFIED = UINT32_MAX;
constexpr uint32_t FS_MAXIMUM_IMAGE_DESCRIPTORS = 500;
constexpr uint32_t FS_MAX_LIGHTS = 30;
constexpr uint32_t FS_MAX_FRAMES_IN_FLIGHT = 2;
constexpr uint32_t FS_MAX_FPS = 120;
constexpr int FS_SCENE_LENGTH = 300;
const std::string FS_APP_NAME = "script";
const std::string FS_PYTHON_PACKAGES_DIR = ".venv/lib/python3.12/site-packages";

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    bool graphicsFamilyHasValue = false;
    bool presentFamilyHasValue = false;
    bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

struct PipelineConfigInfo {
    PipelineConfigInfo() = default;
    PipelineConfigInfo(const PipelineConfigInfo&) = delete;
    PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

    std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    VkPipelineViewportStateCreateInfo viewportInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	VkSpecializationInfo fragmentSpecialisationInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
};

class FestiDevice {
public:
#ifdef NDEBUG
  	const bool enableValidationLayers = false;
#else
  	const bool enableValidationLayers = true;
#endif

	FestiDevice(FestiWindow &window);
	~FestiDevice();

	// Not copyable or movable
	FestiDevice(const FestiDevice &) = delete;
	FestiDevice &operator=(const FestiDevice &) = delete;
	FestiDevice(FestiDevice &&) = delete;
	FestiDevice &operator=(FestiDevice &&) = delete;

	VkCommandPool getCommandPool() { return commandPool; }
	VkDevice device() { return device_; }
	VkSurfaceKHR surface() { return surface_; }
	VkQueue graphicsQueue() { return graphicsQueue_; }
	VkQueue presentQueue() { return presentQueue_; }

	SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
	VkFormat findSupportedFormat(
		const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	// Buffer Helper Functions
	void createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer &buffer,
		VkDeviceMemory &bufferMemory);

	void createCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(
		VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

	void createImageWithInfo(
		const VkImageCreateInfo &imageInfo,
		VkMemoryPropertyFlags properties,
		VkImage &image,
		VkDeviceMemory &imageMemory);

	void createGraphicsPipeline(
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		VkShaderModule& vertShaderModule,
		VkShaderModule& fragShaderModule,
		const PipelineConfigInfo& configInfo,
		VkPipeline& pipeline);

	void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
	
	void transitionImageLayout(
		VkImage image, 
		VkCommandBuffer cmdBuffer,
		VkImageLayout oldLayout, 
		VkImageLayout newLayout, 
		VkAccessFlags src, 
		VkAccessFlags dst,  
		VkPipelineStageFlags pipeFlagsSrc,
		VkPipelineStageFlags pipeFlagsDst);

	static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
	static void defaultImageCreateInfo(VkImageCreateInfo& imageCreateInfo);
	static void defaultImageViewCreateInfo(VkImageViewCreateInfo& imageViewCreateInfo);
	static void defaultSamplerCreateInfo(VkSamplerCreateInfo& samplerCreateInfo);

	VkPhysicalDeviceProperties properties;

private:
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createCommandPool();

	// helper functions
	bool isDeviceSuitable(VkPhysicalDevice device);
	std::vector<const char *> getRequiredExtensions();
	bool checkValidationLayerSupport();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
	void hasGflwRequiredInstanceExtensions();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	FestiWindow &window;
	VkCommandPool commandPool;

	VkDevice device_;
	VkSurfaceKHR surface_;
	VkQueue graphicsQueue_;
	VkQueue presentQueue_;

	const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
		// VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
		};
};

}  // namespace festi
