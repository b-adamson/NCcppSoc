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

class FestiPointLight;
using FS_PointLight = std::shared_ptr<FestiPointLight>;
using FS_PointLightMap = std::unordered_map<uint32_t, FS_PointLight>;

class FestiWorld;
using FS_World = std::shared_ptr<FestiWorld>;

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
    // Explicitly define constructors since we have explicitly defined copy constructor to avoid memory issues (ObjFaceData is in 
    // a std::vector in model, unlike other keyframeable properties)
    ObjFaceData() = default;
    ObjFaceData(uint32_t matID, float sat, float con, glm::vec2 uv)
        : materialID(matID), saturation(sat), contrast(con), uvOffset(uv) {}
    // For pybind only \/\/\/\/\/
    ObjFaceData(uint32_t matID, float sat, float con, std::vector<float> uv)
        : materialID(matID), saturation(sat), contrast(con), uvOffset(uv[0], uv[1]) {}
    ObjFaceData(const ObjFaceData& other) = default;
    ObjFaceData(ObjFaceData&& other) noexcept = default;
    ObjFaceData& operator=(const ObjFaceData& other) {
        if (this != &other) {
            materialID = other.materialID;
            saturation = other.saturation;
            contrast = other.contrast;
            uvOffset = other.uvOffset;
        }
        return *this;
    }
    ObjFaceData& operator=(ObjFaceData&& other) noexcept {
        if (this != &other) {
            saturation = std::move(other.saturation);
            contrast = std::move(other.contrast);
            uvOffset = std::move(other.uvOffset);
            materialID = std::move(other.materialID);
        }
        return *this;
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

    MaterialsSSBO& getMssbo() {return Mssbo;}

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
        uint32_t height);

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
