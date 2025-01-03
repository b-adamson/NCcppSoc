#include "renderer.hpp"

// std
#include <array>
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace festi {

FestiRenderer::FestiRenderer(FestiWindow& window, FestiDevice& device)
    : festiWindow{window}, festiDevice{device} {
	recreateSwapChain();
	commandBuffers.resize(FS_MAX_FRAMES_IN_FLIGHT);
	festiDevice.createCommandBuffers(commandBuffers);
}

FestiRenderer::~FestiRenderer() {
    vkFreeCommandBuffers(
		festiDevice.device(),
		festiDevice.getCommandPool(),
		static_cast<uint32_t>(commandBuffers.size()),
		commandBuffers.data());
	commandBuffers.clear();
	for (size_t i = 0; i < shadowImages.size(); i++) {
        vkDestroyImageView(festiDevice.device(), shadowImageViews[i], nullptr);
        vkDestroyImage(festiDevice.device(), shadowImages[i], nullptr);
        vkFreeMemory(festiDevice.device(), shadowImageMemorys[i], nullptr);
    }
	for (auto framebuffer : shadowFramebuffers) {
        vkDestroyFramebuffer(festiDevice.device(), framebuffer, nullptr);
    }
	vkDestroyRenderPass(festiDevice.device(), shadowRenderPass, nullptr);
	vkDestroySampler(festiDevice.device(), shadowSampler, nullptr);
}

void FestiRenderer::recreateSwapChain() {
	auto extent = festiWindow.getExtent();
	while (extent.width == 0 || extent.height == 0) {
		extent = festiWindow.getExtent();
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(festiDevice.device());

	if (festiSwapChain == nullptr) {
		festiSwapChain = std::make_unique<FestiSwapChain>(festiDevice, extent);
	} else {
		std::shared_ptr<FestiSwapChain> oldSwapChain = std::move(festiSwapChain);
		festiSwapChain = std::make_unique<FestiSwapChain>(festiDevice, extent, oldSwapChain);

		if (!oldSwapChain->compareSwapFormats(*festiSwapChain.get())) {
			throw std::runtime_error("Swap chain image(or depth) format has changed!");
		}
	}
}

VkCommandBuffer FestiRenderer::beginFrame() {
	assert(!isFrameStarted && "Can't call beginFrame while already in progress");

	auto result = festiSwapChain->acquireNextImage(&currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	isFrameStarted = true;

	auto commandBuffer = getCurrentCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	return commandBuffer;
}

void FestiRenderer::endFrame() {
	assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
	auto commandBuffer = getCurrentCommandBuffer();
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	auto result = festiSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
		festiWindow.wasWindowResized()) {
		festiWindow.resetWindowResizedFlag();
		recreateSwapChain();
	} else if (result != VK_SUCCESS) {throw std::runtime_error("failed to present swap chain image!");}

	isFrameStarted = false;
	currentFrameIndex = (currentFrameIndex + 1) % FS_MAX_FRAMES_IN_FLIGHT;
}

void FestiRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
	assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
	assert(
		commandBuffer == getCurrentCommandBuffer() &&
		"Can't begin render pass on command buffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = festiSwapChain->getRenderPass();
	renderPassInfo.framebuffer = festiSwapChain->getFrameBuffer(currentImageIndex);

	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = festiSwapChain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clearValues[1].depthStencil = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(festiSwapChain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(festiSwapChain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{{0, 0}, festiSwapChain->getSwapChainExtent()};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void FestiRenderer::beginShadowPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "Can't call beginShadowPass if frame is not in progress");
    assert(
        commandBuffer == getCurrentCommandBuffer() &&
        "Can't begin shadow pass on command buffer from a different frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = shadowRenderPass;
    renderPassInfo.framebuffer = shadowFramebuffers[currentImageIndex];

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = VkExtent2D{2048, 2048};

    VkClearValue clearValue{};
    clearValue.depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(VkExtent2D{2048, 2048}.width);
    viewport.height = static_cast<float>(VkExtent2D{2048, 2048}.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, VkExtent2D{2048, 2048}};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void FestiRenderer::endShadowPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "Can't call endShadowPass if frame is not in progress");
    assert(
        commandBuffer == getCurrentCommandBuffer() &&
        "Can't end shadow pass on command buffer from a different frame");
    vkCmdEndRenderPass(commandBuffer);
	transitionShadowMapToReadOnly(commandBuffer);
}

void FestiRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
	assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
	assert(
		commandBuffer == getCurrentCommandBuffer() &&
		"Can't end render pass on command buffer from a different frame");
	vkCmdEndRenderPass(commandBuffer);

	transitionShadowMapToAttachment(commandBuffer);
}

void FestiRenderer::transitionShadowMapToReadOnly(VkCommandBuffer cmdBuffer) {
	for (size_t i = 0; i < getImageCount(); i++) {
		festiDevice.transitionImageLayout(
			shadowImages[i],
			cmdBuffer,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT ,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		);
	}
}

void FestiRenderer::transitionShadowMapToAttachment(VkCommandBuffer cmdBuffer) {
	for (size_t i = 0; i < getImageCount(); i++) {
		festiDevice.transitionImageLayout(
			shadowImages[i],
			cmdBuffer,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
		);
	}
}

void FestiRenderer::createShadowItems() {
	createShadowResources();
	createShadowRenderPass();
	createShadowFrameBuffers();
	createShadowSampler();
}

void FestiRenderer::createShadowRenderPass() {
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 0;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency shadowDependency{};
    shadowDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    shadowDependency.dstSubpass = 0;
    shadowDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    shadowDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    shadowDependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    shadowDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 1> shadowAttachments = {depthAttachment};
    VkRenderPassCreateInfo shadowRenderPassInfo{};
    shadowRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    shadowRenderPassInfo.attachmentCount = 1;
    shadowRenderPassInfo.pAttachments = shadowAttachments.data();
    shadowRenderPassInfo.subpassCount = 1;
    shadowRenderPassInfo.pSubpasses = &subpass;
    shadowRenderPassInfo.dependencyCount = 1;
    shadowRenderPassInfo.pDependencies = &shadowDependency;

    if (vkCreateRenderPass(festiDevice.device(), &shadowRenderPassInfo, nullptr, &shadowRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow render pass!");
    }
}

void FestiRenderer::createShadowFrameBuffers() {
    shadowFramebuffers.resize(getImageCount());

    for (size_t i = 0; i < shadowFramebuffers.size(); i++) {
		std::array<VkImageView, 1> attachments = {shadowImageViews[i]};
        VkFramebufferCreateInfo shadowFramebufferInfo{};
        shadowFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        shadowFramebufferInfo.renderPass = shadowRenderPass;
        shadowFramebufferInfo.attachmentCount = 1;
        shadowFramebufferInfo.pAttachments = attachments.data();
        shadowFramebufferInfo.width = 2048;
        shadowFramebufferInfo.height = 2048;
        shadowFramebufferInfo.layers = 1;

        if (vkCreateFramebuffer(festiDevice.device(), &shadowFramebufferInfo, nullptr, &shadowFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow framebuffer!");
        }
    }
}

void FestiRenderer::createShadowResources() {
	VkFormat shadowFormat = festiSwapChain->findDepthFormat();  // Typically, depth format for shadows

    shadowImages.resize(getImageCount());
    shadowImageMemorys.resize(getImageCount());
    shadowImageViews.resize(getImageCount());

    for (size_t i = 0; i < shadowImages.size(); i++) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = 2048;
        imageInfo.extent.height = 2048;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = shadowFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        festiDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            shadowImages[i],
            shadowImageMemorys[i]);

		VkCommandBuffer cmdBuffer = festiDevice.beginSingleTimeCommands();

		festiDevice.transitionImageLayout(
			shadowImages[i],
			cmdBuffer,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			0,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

		festiDevice.endSingleTimeCommands(cmdBuffer);
	
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = shadowImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = shadowFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(festiDevice.device(), &viewInfo, nullptr, &shadowImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow image view!");
        }
    }
}

void FestiRenderer::createShadowSampler() {
	VkSamplerCreateInfo samplerInfo = {};
	festiDevice.defaultSamplerCreateInfo(samplerInfo);
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; 
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    // Create the sampler
    if (vkCreateSampler(festiDevice.device(), &samplerInfo, nullptr, &shadowSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow sampler!");
    }
}

std::vector<VkDescriptorImageInfo> FestiRenderer::getShadowImageViewDescriptorInfo(size_t index) {
	static bool makeResources = false;
	if (!makeResources) {createShadowItems();}
	makeResources = true;
	return std::vector<VkDescriptorImageInfo>(1, VkDescriptorImageInfo{
		shadowSampler,
		shadowImageViews[index],
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
	});
}

}  // namespace festi
