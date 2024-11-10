#pragma once

#include "model.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace festi {

class FestiCamera {
public:

	FestiCamera(FestiWindow& window);

    void updateCameraFromKeyPresses(float dt);

    const glm::mat4 getProjection() const { return projectionMatrix; }
    glm::mat4 getView() const;
    glm::mat4 getInverseView() const;

	void setOrthographicProjection(
        float left, float right, float bottom, float top, float near, float far);
    void setPerspectiveProjection(float fovy, float aspect, float near, float far);

	FestiModel::Transform transform;

private:

	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

	static double mousedy;
	static double mousedx;

	glm::mat4 projectionMatrix{1.f};

	FestiWindow& festiWindow;

	static VkExtent2D activeWindowExtent;

	static int frameRate;
	static float moveSpeed;
	float lookSpeed{0.04f};
};

}  // namespace festi
