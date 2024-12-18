#include "materials.hpp"

#include "utils.hpp"

// lib
#include <stdexcept>
#include <iostream>
#include <unordered_map>

namespace festi {

uint32_t FestiMaterials::appendTextureMap(tinyobj::material_t& matData, const std::string& imgDirPath,const FS_ImageMapFlags flag) {
	std::string name;
	VkFormat format;
	switch (flag) {
	case FS_ImageMapFlags::DIFFUSE:
		name = std::string(matData.diffuse_texname);
		format = VK_FORMAT_R8G8B8A8_SRGB;
		break;
	case FS_ImageMapFlags::NORMAL:
		name = std::string(matData.normal_texname);
		format = VK_FORMAT_R8G8B8A8_UNORM;
		break;
	case FS_ImageMapFlags::SPECULAR:
		name = std::string(matData.specular_texname);
		format = VK_FORMAT_R8G8B8A8_UNORM;
		break;
	default:
		assert(false && "No image map specified");
		break;
	}

	uint32_t idx;
	if (!name.empty()) {
		if (imageViews.find(name) == imageViews.end()) {
			idx = (uint32_t)imageViews.size();
			std::string path = imgDirPath + "/" + name + ".png";
			imageViews[name] = std::pair(idx, createImageViewFromFile(path, format));
		} else {
			idx = imageViews[name].first;
		}
		return idx;
	}
	return FS_UNSPECIFIED;
}

FestiMaterials::FestiMaterials(FestiDevice& device) : festiDevice{device} {
    createDiffuseSampler();
}

void FestiMaterials::writeImageDataToGPU(FestiDevice& device, VkImage image, const std::vector<uint8_t>& imageData, uint32_t width, uint32_t height) {
    VkDeviceSize bufferSize = imageData.size();

    festi::FestiBuffer stagingBuffer(
        device,
        bufferSize,
        1,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer.writeToBuffer((void*)imageData.data(), bufferSize);
    device.copyBufferToImage(stagingBuffer.getBuffer(), image, width, height, 1);
}

// Function to create image views from a list of file paths
VkImageView FestiMaterials::createImageViewFromFile(
    const std::string& filePath, VkFormat format) {

    uint32_t width, height;
    std::vector<uint8_t> imageData;

    // Load image data from file
    if (!loadImageFromFile(filePath, width, height, imageData)) {
        throw std::runtime_error("Failed to load image from file: " + filePath);
    }

    VkDeviceMemory imageMemory;

    // Create Image
    VkImageCreateInfo imageCreateInfo{};
    festiDevice.defaultImageCreateInfo(imageCreateInfo);
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    VkImage image;
    festiDevice.createImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

    // Create Image View
    writeImageDataToGPU(festiDevice, image, imageData, width, height);
    VkImageViewCreateInfo viewCreateInfo{};
    festiDevice.defaultImageViewCreateInfo(viewCreateInfo);
    viewCreateInfo.image = image;
    viewCreateInfo.format = format;
    VkImageView imageView;
    if (vkCreateImageView(festiDevice.device(), &viewCreateInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }
    imageMemories.push_back(imageMemory);
    images.push_back(image);
    return imageView;
}

void FestiMaterials::createDiffuseSampler() {
    VkSamplerCreateInfo samplerInfo{};
    festiDevice.defaultSamplerCreateInfo(samplerInfo);
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    
    vkCreateSampler(festiDevice.device(), &samplerInfo, nullptr, &diffuseSampler);
}

std::vector<VkDescriptorImageInfo> FestiMaterials::getImageViewsDescriptorInfo() {
    std::vector<VkDescriptorImageInfo> descriptorImageInfos;
	descriptorImageInfos.resize(imageViews.size());
	// Remember diffuseImageViews type is string : std::pair(uint32_t, VkImageView)
    for (auto& imageView : imageViews) {
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageView = imageView.second.second;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = diffuseSampler;

        descriptorImageInfos[(size_t)imageView.second.first] = imageInfo;
    }
    return descriptorImageInfos;
}

std::vector<uint32_t> FestiMaterials::getSpecialisationConstants() {
    return std::vector<uint32_t>{
        1000, 1000, 1000};
}

FestiMaterials::~FestiMaterials() {
    for (auto& it : imageViews) {vkDestroyImageView(festiDevice.device(), it.second.second, nullptr);}
    for (auto& it : imageMemories) {vkFreeMemory(festiDevice.device(), it, nullptr);}
    for (auto& it : images) {vkDestroyImage(festiDevice.device(), it, nullptr);}
    vkDestroySampler(festiDevice.device(), diffuseSampler, nullptr);
}

void MaterialsSSBO::appendMaterialFaceIDs(FS_ModelMap& gameObjects) {
	std::fill(objFaceData, objFaceData + 65536, ObjFaceData{});
	size_t offset = 0;
    for (uint32_t i = 0; i < gameObjects.size(); i++) {
		offsets.push_back(offset);
		if (!gameObjects[i]->hasVertexBuffer) {continue;}
		auto IDs =  gameObjects[i]->faceData;
		std::copy(IDs.begin(), IDs.end(), objFaceData + offset);
		offset += IDs.size();
    }
}	

} // namespace festi
