#pragma once

#include "model.hpp"
#include "camera.hpp"

// std
#include <cstddef>
#include <functional>
#include <string>
#include <map>
#include <cstdint>

namespace festi {

inline void hashCombine(std::size_t& seed) { (void)seed; }
template <typename T, typename... Rest>
inline void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hashCombine(seed, rest...);
};

std::vector<char> readFile(const std::string& filepath);

bool loadImageFromFile(const std::string& filePath, uint32_t& width, uint32_t& height, std::vector<uint8_t>& imageData);

struct PointLight {
	glm::vec4 position{};  // ignore w
	glm::vec4 color{};     // w is intensity
};

struct GlobalUBO {
	glm::mat4 projection{1.f};
	glm::mat4 view{1.f};
	glm::mat4 inverseView{1.f};
	glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .2f};  // w is intensity
	glm::vec4 directionalColour{0.f, 0.f, 0.f, 0.f};  // w is intensity
	glm::mat4 lightProjection;
	glm::mat4 lightView;
	PointLight pointLights[FS_MAX_LIGHTS];
	uint32_t pointLightCount;
};

struct FrameInfo {
	uint32_t frameIndex;
	float frameTime;
	VkCommandBuffer commandBuffer;
	FestiCamera& camera;
	FestiCamera& mainLightSource;
	VkDescriptorSet globalSet;
	VkDescriptorSet materialSet;
	VkDescriptorSet shadowMapSet;
	FS_ModelMap& gameObjects;
	FS_PointLightMap& pointLights;
};

} // namespace festi
