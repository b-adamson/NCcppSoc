#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
namespace festi {

class FestiWindow {
public:
	FestiWindow(int w = 1920, int h = 1080, std::string name = "Festi");
	~FestiWindow();

	FestiWindow(const FestiWindow &) = delete;
	FestiWindow &operator=(const FestiWindow &) = delete;

	bool shouldClose() { return glfwWindowShouldClose(window); }
	VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
	bool wasWindowResized() { return framebufferResized; }
	void resetWindowResizedFlag() { framebufferResized = false; }
	GLFWwindow *getGLFWwindow() const { return window; }
	void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

	int frameRate = 60;
	bool inWindow = false;

private:
	static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
	void initWindow();

	int width;
	int height;
	bool framebufferResized = false;

	std::string windowName;
	GLFWwindow *window;
};
}  // namespace festi
