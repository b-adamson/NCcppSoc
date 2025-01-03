#pragma once

#include "device.hpp"
#include "model.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace festi {

class FestiDescriptorSetLayout {
public:
	class Builder {
	public:
		Builder(FestiDevice &festiDevice) : festiDevice{festiDevice} {}

		Builder &addBinding(
			uint32_t binding,
			VkDescriptorType descriptorType,
			VkShaderStageFlags stageFlags,
			uint32_t count = 1);
		FestiDescriptorSetLayout build() const;

	private:
		FestiDevice &festiDevice;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
	};

	FestiDescriptorSetLayout(
		FestiDevice &festiDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
	~FestiDescriptorSetLayout();
	FestiDescriptorSetLayout(const FestiDescriptorSetLayout &) = delete;
	FestiDescriptorSetLayout &operator=(const FestiDescriptorSetLayout &) = delete;

  	VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

private:
	FestiDevice &festiDevice;
	VkDescriptorSetLayout descriptorSetLayout;
	std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

	friend class FestiDescriptorWriter;
};

class FestiDescriptorPool {
public:
	class Builder {
	public:
		Builder(FestiDevice &festiDevice) : festiDevice{festiDevice} {}

		Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
		Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
		Builder &setMaxSets(uint32_t count);
		std::unique_ptr<FestiDescriptorPool> build() const;

	private:
		FestiDevice &festiDevice;
		std::vector<VkDescriptorPoolSize> poolSizes{};
		uint32_t maxSets = 1000;
		VkDescriptorPoolCreateFlags poolFlags = 0;
  	};

	FestiDescriptorPool(
		FestiDevice &festiDevice,
		uint32_t maxSets,
		VkDescriptorPoolCreateFlags poolFlags,
		const std::vector<VkDescriptorPoolSize> &poolSizes);
	~FestiDescriptorPool();
	FestiDescriptorPool(const FestiDescriptorPool &) = delete;
	FestiDescriptorPool &operator=(const FestiDescriptorPool &) = delete;

	bool allocateDescriptorSet(
		const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptorSet) const;

	void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

	void resetPool();

private:
	FestiDevice &festiDevice;
	VkDescriptorPool descriptorPool;

	friend class FestiDescriptorWriter;
};

class FestiDescriptorWriter {
public:
	FestiDescriptorWriter(FestiDescriptorSetLayout& setLayout, FestiDescriptorPool& pool);
	
	FestiDescriptorWriter &writeBuffer(
		uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
	FestiDescriptorWriter &writeImageViews(
		uint32_t binding, std::vector<VkDescriptorImageInfo>& imageInfo);
	FestiDescriptorWriter& writeSampler(
    	uint32_t binding, VkDescriptorImageInfo& samplerInfo);

	bool build(VkDescriptorSet &set);
	void overwrite(VkDescriptorSet &set);

private:
	FestiDescriptorSetLayout &setLayout;
	FestiDescriptorPool &pool;
	std::vector<VkWriteDescriptorSet> writes;
};

}  // namespace festi
