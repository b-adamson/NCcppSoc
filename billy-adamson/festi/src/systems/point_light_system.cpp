#include "point_light_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <map>
#include <stdexcept>
#include <iostream>

namespace festi {

PointLightSystem::PointLightSystem(
		FestiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
		: festiDevice{device} {
	createPipelineLayout(globalSetLayout);
	createPipeline(renderPass);
}

PointLightSystem::~PointLightSystem() {
	vkDestroyPipeline(festiDevice.device(), pointLightPipeline, nullptr);
	vkDestroyPipelineLayout(festiDevice.device(), pipelineLayout, nullptr);
	vkDestroyShaderModule(festiDevice.device(), vertShaderModule, nullptr);
	vkDestroyShaderModule(festiDevice.device(), fragShaderModule, nullptr);	
}

void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PointLightPushConstants);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	if (vkCreatePipelineLayout(festiDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
			VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void PointLightSystem::createPipeline(VkRenderPass renderPass) {
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	FestiDevice::defaultPipelineConfigInfo(pipelineConfig);

	pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
  	pipelineConfig.colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT;
	pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	pipelineConfig.attributeDescriptions.clear(); 
	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	festiDevice.createGraphicsPipeline(
		"bin/point_light.vert.spv",
		"bin/point_light.frag.spv",
		vertShaderModule,
		fragShaderModule,
		pipelineConfig,
		pointLightPipeline);
}

void PointLightSystem::writePointLightsToUBO(FrameInfo& frameInfo, GlobalUBO& ubo) {
	uint32_t lightIndex = 0;
	for (auto& kv : frameInfo.gameObjects) {
		auto& obj = kv.second;
		if (obj->pointLight == nullptr || !obj->visibility) continue;
		assert(lightIndex < FS_MAX_LIGHTS && "Point lights exceed maximum specified");

		// copy light to ubo
		ubo.pointLights[lightIndex].position = glm::vec4(obj->transform.translation, 1.f);
		ubo.pointLights[lightIndex].color = glm::vec4(obj->pointLight->color);

		lightIndex += 1;
	}
	ubo.pointLightCount = lightIndex;
}

void PointLightSystem::renderPointLights(FrameInfo& frameInfo) {
	// Sort lights
	std::map<float, unsigned int> sorted;
	auto camPosition = glm::vec3(frameInfo.camera.getInverseView()[3]);
	for (auto& kv : frameInfo.gameObjects) {
		auto& obj = kv.second;
		if (obj->pointLight == nullptr || !obj->visibility) continue;

		// Calculate distance
		auto offset = camPosition - obj->transform.translation;
		float disSquared = glm::dot(offset, offset);
		sorted[disSquared] = obj->getId();
	}

	vkCmdBindPipeline(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pointLightPipeline);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1,
		&frameInfo.globalSet,
		0,
		nullptr);

	// iterate through sorted lights in reverse order
	for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
		// use game obj id to find light object
		auto& obj = frameInfo.gameObjects.at(it->second);
		if (!obj->visibility) continue;

		PointLightPushConstants push{};
		push.position = glm::vec4(obj->transform.translation, 1.f);
		push.colour = glm::vec4(obj->pointLight->color);
		push.radius = obj->transform.scale.x;

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(PointLightPushConstants),
			&push);
		vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
	}
}

}  // namespace festi
