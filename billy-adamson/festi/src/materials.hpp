#pragma once

#include "buffer.hpp"

// lib
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_obj_loader.h>

// std
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <optional>
#include <random>

namespace festi {

class FestiModel;
using FS_ImageMap = std::unordered_map<std::string, std::pair<uint32_t, VkImageView>>;
using FS_Model = std::shared_ptr<FestiModel>;
using FS_ModelMap = std::unordered_map<uint32_t, FS_Model>;

enum class FS_ImageMapFlags {
    DIFFUSE,
    NORMAL,
    SPECULAR
};

struct Material {
    glm::vec4 diffuseColor = {255.f, 0.f, 255.f, 1.f};
    glm::vec4 specularColor = {255.f, 0.f, 255.f, 1.f};
    float shininess = 32.f;
    uint32_t diffuseTextureIndex;
    uint32_t specularTextureIndex;
    uint32_t normalTextureIndex;  
};

struct ObjFaceData {
    uint32_t materialID = FS_UNSPECIFIED;
    float saturation = 1.f;
    float contrast = 1.f;
    alignas(8) glm::vec2 uvOffset = {.0f, .0f};
    
    bool operator==(const ObjFaceData& other) const {
        return materialID == other.materialID &&
            saturation == other.saturation &&
            contrast == other.contrast &&
            uvOffset == other.uvOffset;
    }

    bool operator!=(const ObjFaceData& other) const {
        return !(*this == other);
    }
};

struct MaterialsSSBO {
    ObjFaceData objFaceData[65536] = {};
    Material materials[200];

    void appendMaterialFaceIDs(FS_ModelMap& gameObjects);
    static std::vector<uint32_t> offsets;
};

class FestiMaterials {
public:
    FestiMaterials(FestiDevice& device);
    ~FestiMaterials();
    FestiMaterials(const FestiMaterials &) = delete;
    FestiMaterials &operator=(const FestiMaterials &) = delete;
    FestiMaterials(FestiMaterials &&) noexcept = delete;
    FestiMaterials &operator=(FestiMaterials &&) noexcept = delete;

    MaterialsSSBO& getMssbo() { return Mssbo; }

    VkImageView createImageViewFromFile(
        const std::string& filePath, 
        VkFormat format);

    std::vector<VkDescriptorImageInfo> getImageViewsDescriptorInfo();

    std::vector<uint32_t> getSpecialisationConstants();
    
private:
    // helpers
    void writeImageDataToGPU(
        FestiDevice& device, 
        VkImage image, 
        const std::vector<uint8_t>& imageData, 
        uint32_t width, 
        uint32_t height);;

    uint32_t appendTextureMap(tinyobj::material_t& matData, const std::string& imgDirPath, const FS_ImageMapFlags flag);

    void createDiffuseSampler();

    FestiDevice& festiDevice;

    MaterialsSSBO Mssbo;

    VkSampler diffuseSampler;
    FS_ImageMap imageViews;
    std::vector<VkDeviceMemory> imageMemories;
    std::vector<VkImage> images;

    static std::unordered_map<std::string, uint32_t> materialNamesMap;

    friend class FestiModel;
};

}
