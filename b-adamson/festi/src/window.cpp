#include "window.hpp"

// std
#include <stdexcept>

namespace festi {

FestiWindow::FestiWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name} {
    initWindow();
}

FestiWindow::~FestiWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void FestiWindow::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void FestiWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to craete window surface");
    }
}

void FestiWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto festiWindow = reinterpret_cast<FestiWindow *>(glfwGetWindowUserPointer(window));
    festiWindow->framebufferResized = true;
    festiWindow->width = width;
    festiWindow->height = height;
}

}  // namespace festi
