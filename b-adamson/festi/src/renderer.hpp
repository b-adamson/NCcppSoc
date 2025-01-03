#pragma once

#include "device.hpp"
#include "swap_chain.hpp"

// std
#include <cassert>
#include <memory>
#include <vector>

namespace festi {
class FestiRenderer {
public:
	FestiRenderer(FestiWindow &window, FestiDevice &device);
	~FestiRenderer();
	FestiRenderer(const FestiRenderer &) = delete;
	FestiRenderer &operator=(const FestiRenderer &) = delete;
	FestiRenderer(FestiRenderer &&) = delete;
	FestiRenderer &operator=(FestiRenderer &&) = delete;

	VkRenderPass getSwapChainRenderPass() const { return festiSwapChain->getRenderPass(); }
	VkRenderPass getSwapChainShadowRenderPass() const { return shadowRenderPass; }
	float getAspectRatio() const { return festiSwapChain->extentAspectRatio(); }
	bool isFrameInProgress() const { return isFrameStarted; }
	VkCommandBuffer getCurrentCommandBuffer() const {
		assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
		return commandBuffers[currentFrameIndex];}
	size_t getImageCount() const {return festiSwapChain->imageCount();}
	uint32_t getFrameBufferIdx() const {
		assert(isFrameStarted && "Cannot get frame index when frame not in progress");
		return currentFrameIndex;}
	std::vector<VkDescriptorImageInfo> getShadowImageViewDescriptorInfo(size_t index);

	VkCommandBuffer beginFrame();
	void endFrame();
	void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void beginShadowPass(VkCommandBuffer commandBuffer);
	void endShadowPass(VkCommandBuffer commandBuffer);

	// Shadow resouces not managed by swapchain
	void createShadowItems();
	void createShadowResources();
	void createShadowRenderPass();
	void createShadowFrameBuffers();
	void createShadowSampler();

	// helper
	void transitionShadowMapToReadOnly(VkCommandBuffer cmdBuffer);
	void transitionShadowMapToAttachment(VkCommandBuffer cmdBuffer);

private:
	void recreateSwapChain();

	FestiWindow &festiWindow;
	FestiDevice &festiDevice;
	std::unique_ptr<FestiSwapChain> festiSwapChain;
	std::vector<VkCommandBuffer> commandBuffers;

	uint32_t currentImageIndex;
	uint32_t currentFrameIndex{0};
	bool isFrameStarted{false};

	// Shadow pass resources
    VkRenderPass shadowRenderPass;
    std::vector<VkFramebuffer> shadowFramebuffers;
    std::vector<VkImage> shadowImages;
    std::vector<VkDeviceMemory> shadowImageMemorys;
    std::vector<VkImageView> shadowImageViews;
	VkSampler shadowSampler;

};

}  // namespace festi
