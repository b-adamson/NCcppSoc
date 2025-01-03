#include "utils.hpp"

// libs
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// std
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>

namespace festi {

std::vector<char> readFile(const std::string& filepath) {
	std::ifstream file{filepath, std::ios::ate | std::ios::binary};

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file: " + filepath);
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

bool loadImageFromFile(const std::string& filePath, uint32_t& width, uint32_t& height, std::vector<uint8_t>& imageData) {
    int w, h, c;
    stbi_uc* data = stbi_load(filePath.c_str(), &w, &h, &c, STBI_rgb_alpha);
    if (!data) {return false;}
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
    imageData.assign(data, data + (w * h * 4)); // Assuming RGBA format
    stbi_image_free(data);

    return true;
}

bool runOnceIfKeyPressed(FestiWindow& window, int key, std::function<void()> onPress) {
	static std::unordered_map<int, bool> keyWasPressedMap;
	bool& keyWasPressed = keyWasPressedMap[key];

	if (glfwGetKey(window.getGLFWwindow(), key) == GLFW_PRESS) {
		if (keyWasPressed == false) {
			onPress();
			keyWasPressed = true;
			return true; 
		}
	} else {
		keyWasPressed = false;
	}
	return false;
};

} // namespace festi
