#include "buffer.hpp"

// std
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>

namespace festi {

VkDeviceSize FestiBuffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
	if (minOffsetAlignment > 0) {
		return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
	}
	return instanceSize;
}

FestiBuffer::FestiBuffer(
    FestiDevice &device,
    VkDeviceSize instanceSize,
    uint32_t instanceCount,
    VkBufferUsageFlags useFlags,
    VkMemoryPropertyFlags memPropFlags,
    VkDeviceSize minOffsetAlignment
    ):  festiDevice{device},
		instanceSize{instanceSize},
		instanceCount{instanceCount},
		usageFlags{useFlags},
		memoryPropertyFlags{memPropFlags} {
	alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
	bufferSize = alignmentSize * instanceCount;
	device.createBuffer(bufferSize, useFlags, memPropFlags, buffer, memory);
	if (memPropFlags != VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) map(); 
}

FestiBuffer::~FestiBuffer() {
	unmap();
	vkDestroyBuffer(festiDevice.device(), buffer, nullptr);
	vkFreeMemory(festiDevice.device(), memory, nullptr);
}

VkResult FestiBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
	assert(buffer && memory && "Called map on buffer before create");
	return vkMapMemory(festiDevice.device(), memory, offset, size, 0, &mapped);
}

void FestiBuffer::unmap() {
	if (mapped) {
		vkUnmapMemory(festiDevice.device(), memory);
		mapped = nullptr;
	}
}

void FestiBuffer::writeToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) {
	assert(mapped != nullptr && "Cannot copy to unmapped buffer");

	if (size == VK_WHOLE_SIZE) {
		memcpy(mapped, data, bufferSize);
	} else {
		char *memOffset = (char*)mapped;
		memOffset += offset;
		memcpy(memOffset, data, size);
	}
}

VkResult FestiBuffer::flush(VkDeviceSize size, VkDeviceSize offset) {
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkFlushMappedMemoryRanges(festiDevice.device(), 1, &mappedRange);
}

VkResult FestiBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkInvalidateMappedMemoryRanges(festiDevice.device(), 1, &mappedRange);
}

VkDescriptorBufferInfo FestiBuffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
  	return VkDescriptorBufferInfo{
		buffer,
		offset,
      	size,
 	};
}

std::unique_ptr<FestiBuffer> FestiBuffer::writeToLocalGPU(
	void* data, 
	FestiDevice& device, 
	VkDeviceSize instanceSize, 
	uint32_t instanceCount, 
	VkBufferUsageFlagBits flags) {

	VkDeviceSize bufferSize = instanceSize * instanceCount;

	FestiBuffer stagingBuffer {
		device,
		instanceSize,
		instanceCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
	stagingBuffer.writeToBuffer(data);

	auto localBuffer = std::make_unique<FestiBuffer>(
		device,
		instanceSize,
		instanceCount,
		flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  	device.copyBuffer(stagingBuffer.getBuffer(), localBuffer->getBuffer(), bufferSize);

	return localBuffer;
}

}  // namespace festi
