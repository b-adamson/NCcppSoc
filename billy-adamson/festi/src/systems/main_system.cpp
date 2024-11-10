#include "main_system.hpp"

#include "model.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

#include <iostream>

namespace festi {

MainSystem::MainSystem(
    FestiDevice& device, 
	FestiRenderer& renderer,
	VkDescriptorSetLayout globalSetLayout, 
	VkDescriptorSetLayout materialsSetLayout,
	VkDescriptorSetLayout shadowMapSetLayout)
    : festiDevice{device} {

	createPipelineLayout(globalSetLayout, materialsSetLayout, shadowMapSetLayout);
	createPipeline(
		renderer.getSwapChainRenderPass(), 
		renderer.getSwapChainShadowRenderPass());
}

MainSystem::~MainSystem() {
	vkDestroyPipeline(festiDevice.device(), mainPipeline, nullptr);
	vkDestroyPipeline(festiDevice.device(), shadowPipeline, nullptr);
	vkDestroyPipelineLayout(festiDevice.device(), mainPipelineLayout, nullptr);
	vkDestroyPipelineLayout(festiDevice.device(), shadowPipelineLayout, nullptr);
	vkDestroyShaderModule(festiDevice.device(), mainVertShaderModule, nullptr);
	vkDestroyShaderModule(festiDevice.device(), mainFragShaderModule, nullptr);
	vkDestroyShaderModule(festiDevice.device(), shadowVertShaderModule, nullptr);
	vkDestroyShaderModule(festiDevice.device(), shadowFragShaderModule, nullptr);
}

void MainSystem::createPipelineLayout(
		VkDescriptorSetLayout globalSetLayout, 
		VkDescriptorSetLayout materialsSetLayout,
		VkDescriptorSetLayout shadowMapSetLayout) {
	// MAIN
	VkPushConstantRange mainPushConstantRange{};
	mainPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	mainPushConstantRange.offset = 0;
	mainPushConstantRange.size = sizeof(MainPushConstants);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
		globalSetLayout, materialsSetLayout, shadowMapSetLayout};

	VkPipelineLayoutCreateInfo mainPipelineLayoutInfo{};
	mainPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	mainPipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
	mainPipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	mainPipelineLayoutInfo.pushConstantRangeCount = 1;
	mainPipelineLayoutInfo.pPushConstantRanges = &mainPushConstantRange;
	if (vkCreatePipelineLayout(festiDevice.device(), &mainPipelineLayoutInfo, nullptr, &mainPipelineLayout) !=
		VK_SUCCESS) {throw std::runtime_error("failed to create pipeline layout!");}

	// SHADOW
	VkPushConstantRange shadowPushConstantRange{};
	shadowPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	shadowPushConstantRange.offset = 0;
	shadowPushConstantRange.size = sizeof(ShadowPushConstants);

	VkPipelineLayoutCreateInfo shadowPipelineLayoutInfo{};
	shadowPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	shadowPipelineLayoutInfo.setLayoutCount = 0;
	shadowPipelineLayoutInfo.pSetLayouts = nullptr;
	shadowPipelineLayoutInfo.pushConstantRangeCount = 1;
	shadowPipelineLayoutInfo.pPushConstantRanges = &shadowPushConstantRange;
	if (vkCreatePipelineLayout(festiDevice.device(), &shadowPipelineLayoutInfo, nullptr, &shadowPipelineLayout) !=
		VK_SUCCESS) {throw std::runtime_error("failed to create shadow pipeline layout!");}
}

void MainSystem::createPipeline(VkRenderPass renderPass, VkRenderPass shadowRenderPass) {
    assert(mainPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
	// MAIN
    PipelineConfigInfo pipelineConfig{};
    FestiDevice::defaultPipelineConfigInfo(pipelineConfig);

	pipelineConfig.bindingDescriptions = Vertex::getBindingDescriptions();
	pipelineConfig.attributeDescriptions = Vertex::getAttributeDescriptions();

	// VkSpecializationMapEntry mapEntries[3] = {};
	// mapEntries[0].constantID = 0;
	// mapEntries[0].offset = 0;
	// mapEntries[0].size = sizeof(uint32_t);
	// mapEntries[1].constantID = 1;
	// mapEntries[1].offset = sizeof(uint32_t);
	// mapEntries[1].size = sizeof(uint32_t);
	// mapEntries[2].constantID = 2;
	// mapEntries[2].offset = sizeof(uint32_t) * 2;
	// mapEntries[2].size = sizeof(uint32_t);

	// VkSpecializationInfo specializationInfo = {};
	// pipelineConfig.fragmentSpecialisationInfo.mapEntryCount = 3;
	// pipelineConfig.fragmentSpecialisationInfo.pMapEntries = mapEntries;
	// pipelineConfig.fragmentSpecialisationInfo.dataSize = specialisationConstants.size() * sizeof(uint32_t);
	// pipelineConfig.fragmentSpecialisationInfo.pData = specialisationConstants.data();

    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = mainPipelineLayout;
    festiDevice.createGraphicsPipeline(
        "bin/main_shader.vert.spv",
        "bin/main_shader.frag.spv",
        mainVertShaderModule,
        mainFragShaderModule,
        pipelineConfig,
        mainPipeline);

	// SHADOW
	assert(shadowPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
	PipelineConfigInfo shadowPipelineConfig{};
	FestiDevice::defaultPipelineConfigInfo(shadowPipelineConfig);

    shadowPipelineConfig.colorBlendAttachment.colorWriteMask = 0;
    shadowPipelineConfig.colorBlendAttachment.blendEnable = VK_FALSE;

    shadowPipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
 	shadowPipelineConfig.rasterizationInfo.depthBiasEnable = VK_TRUE;

    shadowPipelineConfig.colorBlendInfo.logicOpEnable = VK_FALSE;
    shadowPipelineConfig.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	shadowPipelineConfig.colorBlendInfo.attachmentCount = 0;
	shadowPipelineConfig.colorBlendInfo.pAttachments = nullptr;

	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	bindingDescriptions.push_back({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
	bindingDescriptions.push_back({1, sizeof(Instance), VK_VERTEX_INPUT_RATE_INSTANCE});

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex, position)});
	attributeDescriptions.push_back({1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, modelMatColumn1)});
	attributeDescriptions.push_back({2, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, modelMatColumn2)});
	attributeDescriptions.push_back({3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, modelMatColumn3)});
	attributeDescriptions.push_back({4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance, modelMatColumn4)});

	shadowPipelineConfig.attributeDescriptions = attributeDescriptions;
	shadowPipelineConfig.bindingDescriptions = bindingDescriptions;

	shadowPipelineConfig.renderPass = shadowRenderPass;
	shadowPipelineConfig.pipelineLayout = shadowPipelineLayout;
	festiDevice.createGraphicsPipeline(
		"bin/shadow.vert.spv",
		"bin/shadow.frag.spv",
		shadowVertShaderModule,
		shadowFragShaderModule,
		shadowPipelineConfig,
		shadowPipeline);
}

void MainSystem::renderGameObjects(FrameInfo& frameInfo) {
    vkCmdBindPipeline(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipeline);

    std::vector<VkDescriptorSet> descriptorSets = {frameInfo.globalSet, frameInfo.materialSet, frameInfo.shadowMapSet};
    vkCmdBindDescriptorSets(
        frameInfo.commandBuffer, 
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        mainPipelineLayout,                 
        0,
        (uint32_t)descriptorSets.size(),
        descriptorSets.data(),
        0,
        nullptr);

	for (size_t i = 0; i < frameInfo.gameObjects.size(); i++) {
		auto& obj = frameInfo.gameObjects[i];
		if (!obj->hasVertexBuffer || !obj->visibility) { continue; }

		MainPushConstants push{};
		push.objectID = obj->getId();
		push.offset = MaterialsSSBO::offsets[i];

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			mainPipelineLayout,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(MainPushConstants),
			&push);

		obj->bind(frameInfo.commandBuffer);
		obj->draw(frameInfo.commandBuffer);
  	}
}

void MainSystem::createShadowMap(FrameInfo& frameInfo) {
	vkCmdBindPipeline(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
	ShadowPushConstants push;
	push.lightSpace = frameInfo.mainLightSource.getProjection() * frameInfo.mainLightSource.getView();

	vkCmdPushConstants(
		frameInfo.commandBuffer,
		shadowPipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(ShadowPushConstants),
		&push);

	for (size_t i = 0; i < frameInfo.gameObjects.size(); i++) {
		auto& obj = frameInfo.gameObjects[i];
		if (!obj->hasVertexBuffer || !obj->visibility) { continue; }

		obj->bind(frameInfo.commandBuffer);
		obj->draw(frameInfo.commandBuffer);	
	}
}

}  // namespace festi
