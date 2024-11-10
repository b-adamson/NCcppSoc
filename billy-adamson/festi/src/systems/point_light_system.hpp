#pragma once

#include "camera.hpp"
#include "device.hpp"
#include "utils.hpp"

// std
#include <memory>
#include <vector>

namespace festi {

struct PointLightPushConstants {
	glm::vec4 position{};
	glm::vec4 colour{};
	float radius;
};

class PointLightSystem {
public:
	PointLightSystem(
		FestiDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~PointLightSystem();

	PointLightSystem(const PointLightSystem &) = delete;
	PointLightSystem &operator=(const PointLightSystem &) = delete;

	static void writePointLightsToUBO(FrameInfo &frameInfo, GlobalUBO &ubo);
	void renderPointLights(FrameInfo& frameInfo);

private:
	void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void createPipeline(VkRenderPass renderPass);

	FestiDevice& festiDevice;

	VkPipeline pointLightPipeline;
	VkPipelineLayout pipelineLayout;

	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
};

}  // namespace festi
