#pragma once

#include "buffer.hpp"
#include "materials.hpp"

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
#include <algorithm>
#include <random>

namespace festi {

enum KeyFrameFlags {
    FS_KEYFRAME_POS_ROT_SCALE = 1 << 0,
    FS_KEYFRAME_FACE_MATERIALS = 1 << 1,
    FS_KEYFRAME_POINT_LIGHT = 1 << 2,
    FS_KEYFRAME_AS_INSTANCE = 1 << 3,
    FS_KEYFRAME_WORLD = 1 << 4,
    FS_KEYFRAME_VISIBILITY = 1 << 5
};

inline KeyFrameFlags operator|(KeyFrameFlags lhs, KeyFrameFlags rhs) {
    return static_cast<KeyFrameFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;
    glm::vec2 uv;

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

    bool operator==(const Vertex& other) const {
		return position == other.position && normal == other.normal 
			&& uv == other.uv && tangent == other.tangent && bitangent == other.bitangent;}
};

struct Instance {
	glm::vec4 modelMatColumn1;
	glm::vec4 modelMatColumn2;
	glm::vec4 modelMatColumn3;
	glm::vec4 modelMatColumn4;

	glm::vec3 normalMatColumn1;
	glm::vec3 normalMatColumn2;
	glm::vec3 normalMatColumn3;

	Instance(glm::mat4 modelMat, glm::mat3 normalMat) : 
		modelMatColumn1{modelMat[0]},
		modelMatColumn2{modelMat[1]},
		modelMatColumn3{modelMat[2]},
		modelMatColumn4{modelMat[3]},

		normalMatColumn1{normalMat[0]},
		normalMatColumn2{normalMat[1]},
		normalMatColumn3{normalMat[2]} {};
};

class FestiModel {

template <typename T>
using FS_KeyframeMap_t = std::map<uint32_t, T>;

public:
    struct PointLightComponent {
        glm::vec4 color = {1.f, 1.f, 1.f, 1.f};

        bool operator==(const PointLightComponent& other) {return color == other.color;}

        bool operator!=(const PointLightComponent& other) {return !(*this == other);}
    };

    struct WorldProperties {
        glm::vec4 mainLightColour = {.0f, .0f, .0f, .0f};
        glm::vec2 mainLightDirection = {.0f, .0f};
        glm::vec4 ambientColour = {.1f, .1f, .1f, .1f};

        glm::vec3 getDirectionVector();

        bool operator==(const WorldProperties& other) {
            return mainLightColour == other.mainLightColour && 
                mainLightDirection == other.mainLightDirection && 
                ambientColour == other.ambientColour;}

        bool operator!=(const WorldProperties& other) {return !(*this == other);}
    };

    struct Transform {
		glm::vec3 translation{};
		glm::vec3 scale{1.f, 1.f, 1.f};
		glm::vec3 rotation{};

		glm::mat4 getModelMatrix();
		glm::mat4 getNormalMatrix();

        bool operator==(const Transform& other) const {
            return translation == other.translation && scale == other.scale && rotation == other.rotation;}

        bool operator!=(const Transform& other) const {return !(*this == other);}
    } transform;

    struct AsInstanceData {
        std::shared_ptr<FestiModel> parentObject = nullptr;
        float density = 0.0;
        uint32_t seed = 0;
        float randomness = 1.f;
        Transform minOffset;
        Transform maxOffset;

        void makeStandAlone() {*this = AsInstanceData{};}

        bool operator==(const AsInstanceData& other) const {
            return (parentObject == other.parentObject) && (density == other.density) 
                && (seed == other.seed) && (randomness == other.randomness) 
                && (minOffset == other.minOffset) && (maxOffset == other.maxOffset);}

        bool operator!=(const AsInstanceData& other) const {
            return !(*this == other);}

    } asInstanceData;

    struct KeyFrames {
        // keyframeable properties
        FS_KeyframeMap_t<Transform> transforms; // FS_KEYFRAME_POS_ROT_SCALE
        FS_KeyframeMap_t<std::map<uint32_t, ObjFaceData>> objFaceData; // FS_KEYFRAME_MATERIAL
        FS_KeyframeMap_t<PointLightComponent> pointLightData; // FS_KEYFRAME_POINT_LIGHT
        FS_KeyframeMap_t<AsInstanceData> asInstanceData; // FS_KEYFRAME_AS_INSTANCE
        FS_KeyframeMap_t<WorldProperties> worldProperties; // FS_KEYFRAME_WORLD
        FS_KeyframeMap_t<bool> visibility; // FS_KEYFRAME_VISIBILITY

        // helpers
        std::set<uint32_t> modifiedFaces;
        std::set<int> inMotion;
    } keyframes;

    FestiModel(FestiDevice& device);
    ~FestiModel();

    FestiModel(const FestiModel&) = delete;
    FestiModel &operator=(const FestiModel&) = delete;
    FestiModel(FestiModel &&) noexcept = default;
    FestiModel &operator=(FestiModel &&) noexcept = default;

    static std::shared_ptr<FestiModel> createModelFromFile(
        FestiDevice& device, 
	    FestiMaterials& festiMaterials,
        FS_ModelMap& gameObjects,
        const std::string& filepath,
        const std::string& mtlDir,
        const std::string& imgDir);

    static std::shared_ptr<FestiModel> createPointLight(FestiDevice& device, FS_ModelMap& gameObjects,
        float radius = 0.1f, glm::vec4 color = glm::vec4(1.f));

    static void addObjectToSceneWithName(
        FS_Model& object, FS_ModelMap& gameObjects);

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

    void insertKeyframe(uint32_t idx, KeyFrameFlags flags, std::vector<uint32_t> faceIDs = {0});

    uint32_t getId() {return id;}
    static uint32_t getMaterial(std::string name) {return materialNamesMap[name];}
    uint32_t getNumberOfFaces() {return indexCount / 3;}
    std::vector<Instance> getTransformsToRndPointsOnSurface(const AsInstanceData& asInstanceDataKeyframe, Transform& childTransform);
    std::vector<uint32_t> ALL_FACES() {std::vector<uint32_t> vec(indexCount / 3); std::iota(vec.begin(), vec.end(), 0); return vec;}

    static void setInstanceBufferSizesOnGameObjects(FS_ModelMap& gameObjects);
    void writeToInstanceBuffer(const std::vector<Instance>& instances);

    bool visibility = true;
    std::vector<ObjFaceData> faceData;

    std::shared_ptr<PointLightComponent> pointLight = nullptr;
    std::unique_ptr<WorldProperties> world = nullptr;
    bool hasIndexBuffer = false;
    bool hasVertexBuffer = false;

    float shapeArea = 0;
    
private:
    // helpers
    static void setTangentsBitangentsShapeArea(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, float& area);
    void createVertexBuffer(const std::vector<Vertex>& vertices);
	void createIndexBuffer(const std::vector<uint32_t>& indices);
    void createInstanceBuffer(uint32_t size);

    FestiDevice& festiDevice;

    uint32_t id;

	std::unique_ptr<FestiBuffer> vertexBuffer = nullptr;
    std::vector<Vertex> vertices;
	uint32_t vertexCount;

	std::unique_ptr<FestiBuffer> indexBuffer = nullptr;
    std::vector<uint32_t> indices;
	uint32_t indexCount;

    std::unique_ptr<FestiBuffer> instanceBuffer = nullptr;
    uint32_t instanceCount;

    static std::unordered_map<std::string, uint32_t> materialNamesMap;

    friend class FestiMaterials;
};

} // festi namespace
