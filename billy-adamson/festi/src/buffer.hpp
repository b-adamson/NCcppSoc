#pragma once

#include "device.hpp"

#include <memory>

namespace festi {

class FestiBuffer {
public:
	FestiBuffer(
		FestiDevice& device,
		VkDeviceSize instanceSize,
		uint32_t instanceCount,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VkDeviceSize minOffsetAlignment = 1
		);
	~FestiBuffer();

	FestiBuffer(const FestiBuffer&) = delete;
	FestiBuffer& operator=(const FestiBuffer&) = delete;

	VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void unmap();

	void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

	void writeToIndex(void* data, uint32_t index) {writeToBuffer(data, instanceSize, index * alignmentSize);}
	VkResult flushIndex(uint32_t index) { return flush(alignmentSize, index * alignmentSize); }
	VkDescriptorBufferInfo descriptorInfoForIndex(uint32_t index) {return descriptorInfo(alignmentSize, index * alignmentSize);}
	VkResult invalidateIndex(uint32_t index) {return invalidate(alignmentSize, index * alignmentSize);}

	VkBuffer getBuffer() const { return buffer; }
	void* getMappedMemory() const { return mapped; }
	uint32_t getInstanceCount() const { return instanceCount; }
	VkDeviceSize getInstanceSize() const { return instanceSize; }
	VkDeviceSize getAlignmentSize() const { return instanceSize; }
	VkBufferUsageFlags getUsageFlags() const { return usageFlags; }
	VkMemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; }
	VkDeviceSize getBufferSize() const { return bufferSize; }

	static std::unique_ptr<FestiBuffer> writeToLocalGPU(
		void* data, 
		FestiDevice& device,
		VkDeviceSize instanceSize, 
		uint32_t instanceCount, 
		VkBufferUsageFlagBits flags);
	
private:
	static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

	FestiDevice& festiDevice;
	void* mapped = nullptr;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;

	VkDeviceSize bufferSize;
	VkDeviceSize instanceSize;
	uint32_t instanceCount;
	VkBufferUsageFlags usageFlags;
	VkMemoryPropertyFlags memoryPropertyFlags;
	VkDeviceSize alignmentSize;
};

}  // namespace festi
