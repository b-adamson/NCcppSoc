#pragma once

#include "camera.hpp"
#include "device.hpp"
#include "utils.hpp"
#include "renderer.hpp"

// std
#include <memory>
#include <vector>

namespace festi {

struct MainPushConstants {
	uint32_t objectID;
	uint32_t offset;
};

struct ShadowPushConstants {
	glm::mat4 lightSpace;
};

class MainSystem {
public:
	MainSystem(
		FestiDevice& device, 
		FestiRenderer& renderer,
		VkDescriptorSetLayout globalSetLayout, 
		VkDescriptorSetLayout materialsSetLayout,
		VkDescriptorSetLayout shadowMapSetLayout);
	~MainSystem();

	MainSystem(const MainSystem &) = delete;
	MainSystem &operator=(const MainSystem &) = delete;

	void renderGameObjects(FrameInfo& frameInfo);
	void createShadowMap(FrameInfo& frameInfo);

private:
	void createPipelineLayout(
		VkDescriptorSetLayout globalSetLayout, 
		VkDescriptorSetLayout materialsSetLayout,
		VkDescriptorSetLayout shadowMapSetLayout);
	void createPipeline(
		VkRenderPass renderPass, 
		VkRenderPass shadowRenderPass);

	FestiDevice &festiDevice;

	VkPipeline mainPipeline;
	VkPipeline shadowPipeline;
	VkPipelineLayout mainPipelineLayout;
	VkPipelineLayout shadowPipelineLayout;
	// std::vector<uint32_t> specialisationConstants;

	VkShaderModule mainVertShaderModule;
	VkShaderModule mainFragShaderModule;
	VkShaderModule shadowVertShaderModule;
	VkShaderModule shadowFragShaderModule;
};
}  // namespace festi
