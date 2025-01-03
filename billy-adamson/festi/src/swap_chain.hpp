#pragma once

#include "device.hpp"

// lib
#include <vulkan/vulkan.h>

// std
#include <memory>
#include <string>
#include <vector>

namespace festi {

class FestiSwapChain {
public:

	FestiSwapChain(FestiDevice &deviceRef, VkExtent2D windowExtent);
	FestiSwapChain(
		FestiDevice &deviceRef, VkExtent2D windowExtent, std::shared_ptr<FestiSwapChain> previous);

	~FestiSwapChain();

	FestiSwapChain(const FestiSwapChain &) = delete;
	FestiSwapChain &operator=(const FestiSwapChain &) = delete;

	VkFramebuffer getFrameBuffer(uint32_t index) {return swapChainFramebuffers[index];}
	VkRenderPass getRenderPass() {return renderPass;}
	VkImageView getImageView(uint32_t index) {return swapChainImageViews[index];}
	size_t imageCount() {return swapChainImages.size();}
	VkFormat getSwapChainImageFormat() {return swapChainImageFormat;}
	VkExtent2D getSwapChainExtent() {return swapChainExtent;}
	uint32_t width() {return swapChainExtent.width;}
	uint32_t height() {return swapChainExtent.height;}

	float extentAspectRatio() {
		return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);}
	VkFormat findDepthFormat();

	VkResult acquireNextImage(uint32_t *imageIndex);
	VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

	bool compareSwapFormats(const FestiSwapChain &other) const {
		return other.swapChainDepthFormat == swapChainDepthFormat &&
			other.swapChainImageFormat == swapChainImageFormat;}

private:
	void init();
	void createSwapChain();
	void createImageViews();
	void createDepthResources();
	void createRenderPass();
	void createFramebuffers();
	void createSyncObjects();

	// Helper functions
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR> &availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR> &availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

	VkFormat swapChainImageFormat;
	VkFormat swapChainDepthFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkFramebuffer> swapChainFramebuffers;

	// Main pass resources
	VkRenderPass renderPass;
	std::vector<VkImage> depthImages;
	std::vector<VkDeviceMemory> depthImageMemorys;
	std::vector<VkImageView> depthImageViews;

	// Swapchain image resources
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	FestiDevice &device;
	VkExtent2D windowExtent;

	VkSwapchainKHR swapChain;
	std::shared_ptr<FestiSwapChain> oldSwapChain;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame = 0;
};

}  // namespace festi
