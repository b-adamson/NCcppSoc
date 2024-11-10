#include "descriptors.hpp"

// std
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace festi {

/*************** Descriptor Set Layout Builder *********************/

FestiDescriptorSetLayout::Builder &FestiDescriptorSetLayout::Builder::addBinding(
    uint32_t binding,
    VkDescriptorType descriptorType,
    VkShaderStageFlags stageFlags,
    uint32_t count) {
	assert(bindings.count(binding) == 0 && "Binding already in use");
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = descriptorType;
	layoutBinding.descriptorCount = count;
	layoutBinding.stageFlags = stageFlags;
	bindings[binding] = layoutBinding;
	return *this;
}

FestiDescriptorSetLayout FestiDescriptorSetLayout::Builder::build() const {
 	 return FestiDescriptorSetLayout(festiDevice, bindings);
}

// *************** Descriptor Set Layout *********************

FestiDescriptorSetLayout::FestiDescriptorSetLayout(
    FestiDevice &festiDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
    : festiDevice{festiDevice}, bindings{bindings} {
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
	for (auto kv : bindings) {
		setLayoutBindings.push_back(kv.second);
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(
		festiDevice.device(),
		&descriptorSetLayoutInfo,
		nullptr,
		&descriptorSetLayout) != VK_SUCCESS) {
    	throw std::runtime_error("failed to create descriptor set layout!");
  	}
}

FestiDescriptorSetLayout::~FestiDescriptorSetLayout() {
  	vkDestroyDescriptorSetLayout(festiDevice.device(), descriptorSetLayout, nullptr);
}

// *************** Descriptor Pool Builder *********************

FestiDescriptorPool::Builder &FestiDescriptorPool::Builder::addPoolSize(
    VkDescriptorType descriptorType, uint32_t count) {
	poolSizes.push_back({descriptorType, count});
	return *this;
}

FestiDescriptorPool::Builder &FestiDescriptorPool::Builder::setPoolFlags(
    VkDescriptorPoolCreateFlags flags) {
	poolFlags = flags;
	return *this;
}
FestiDescriptorPool::Builder &FestiDescriptorPool::Builder::setMaxSets(uint32_t count) {
	maxSets = count;
	return *this;
}

std::unique_ptr<FestiDescriptorPool> FestiDescriptorPool::Builder::build() const {
  	return std::make_unique<FestiDescriptorPool>(festiDevice, maxSets, poolFlags, poolSizes);
}

// *************** Descriptor Pool *********************

FestiDescriptorPool::FestiDescriptorPool(
    FestiDevice &festiDevice,
    uint32_t maxSets,
    VkDescriptorPoolCreateFlags poolFlags,
    const std::vector<VkDescriptorPoolSize> &poolSizes)
    : festiDevice{festiDevice} {
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = maxSets;
	descriptorPoolInfo.flags = poolFlags;

	if (vkCreateDescriptorPool(festiDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
      VK_SUCCESS) {
    	throw std::runtime_error("failed to create descriptor pool!");
 	}
}

FestiDescriptorPool::~FestiDescriptorPool() {
 	vkDestroyDescriptorPool(festiDevice.device(), descriptorPool, nullptr);
}

bool FestiDescriptorPool::allocateDescriptorSet(
    const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptorSet) const {
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;
	if (vkAllocateDescriptorSets(festiDevice.device(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
		return false;
	}
	return true;
}

void FestiDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const {
	vkFreeDescriptorSets(
		festiDevice.device(),
		descriptorPool,
		static_cast<uint32_t>(descriptors.size()),
		descriptors.data());
}

void FestiDescriptorPool::resetPool() {
  	vkResetDescriptorPool(festiDevice.device(), descriptorPool, 0);
}

// *************** Descriptor Writer *********************

FestiDescriptorWriter::FestiDescriptorWriter(FestiDescriptorSetLayout &setLayout, FestiDescriptorPool &pool)
    : setLayout{setLayout}, pool{pool} {}

FestiDescriptorWriter& FestiDescriptorWriter::writeBuffer(
    uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {
 	assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

	auto& bindingDescription = setLayout.bindings[binding];

	assert(
		bindingDescription.descriptorCount == 1 &&
		"Binding single descriptor info, but binding expects multiple");

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pBufferInfo = bufferInfo;
	write.descriptorCount = 1;

	writes.push_back(write);
	return *this;
}

FestiDescriptorWriter& FestiDescriptorWriter::writeImageViews(
    uint32_t binding, std::vector<VkDescriptorImageInfo>& imageInfo) {
  	assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

	auto& bindingDescription = setLayout.bindings[binding];

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pImageInfo = imageInfo.data();
	write.descriptorCount = (uint32_t)imageInfo.size();
	write.dstArrayElement = 0;

	writes.push_back(write);
	return *this;
}

FestiDescriptorWriter& FestiDescriptorWriter::writeSampler(
    uint32_t binding, VkDescriptorImageInfo& imageInfo) {
  	assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

	auto& bindingDescription = setLayout.bindings[binding];

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pImageInfo = &imageInfo;
	write.descriptorCount = 1;
	write.dstArrayElement = 0;

	writes.push_back(write);
	return *this;
}

bool FestiDescriptorWriter::build(VkDescriptorSet &set) {
	bool success = pool.allocateDescriptorSet(setLayout.getDescriptorSetLayout(), set);
	if (!success) {
		return false;
	}
	overwrite(set);
	return true;
}

void FestiDescriptorWriter::overwrite(VkDescriptorSet &set) {
	for (auto &write : writes) {
		write.dstSet = set;
	}
	vkUpdateDescriptorSets(pool.festiDevice.device(), writes.size(), writes.data(), 0, nullptr);
}

}  // namespace festi
