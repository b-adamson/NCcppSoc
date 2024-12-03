#include "camera.hpp"

#include "device.hpp"

// std
#include <cassert>
#include <limits>
#include <iostream>
#include <chrono>
#include <thread>

namespace festi {

FestiCamera::FestiCamera(FestiWindow& window) : festiWindow{window} {
	glfwSetScrollCallback(festiWindow.getGLFWwindow(), scroll_callback);
	glfwSetCursorPosCallback(festiWindow.getGLFWwindow(), mouse_callback);
}

void FestiCamera::setOrthographicProjection(
    float left, float right, float bottom, float top, float near, float far) {
    projectionMatrix = glm::mat4{1.0f};
    projectionMatrix[0][0] = 2.0f / (right - left);
    projectionMatrix[1][1] = 2.0f / (top - bottom);
    projectionMatrix[2][2] = 1.0f / (far - near);
    projectionMatrix[3][0] = -(right + left) / (right - left);
    projectionMatrix[3][1] = -(top + bottom) / (top - bottom);
    projectionMatrix[3][2] = -near / (far - near);
}

void FestiCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
	assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
	const float tanHalfFovy = tan(fovy / 2.f);
	projectionMatrix = glm::mat4{1.0f};
	projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
	projectionMatrix[1][1] = 1.f / (tanHalfFovy);
	projectionMatrix[2][2] = far / (far - near);
	projectionMatrix[2][3] = 1.f;
	projectionMatrix[3][2] = -(far * near) / (far - near);
}

glm::mat4 FestiCamera::getView() const {
	const float c2 = glm::cos(transform.rotation.x);
	const float s2 = glm::sin(transform.rotation.x);
	const float c1 = glm::cos(transform.rotation.y);
	const float s1 = glm::sin(transform.rotation.y);
	const glm::vec3 w = glm::vec3((c2 * s1), (-s2), (c1 * c2));
	const glm::vec3 u{glm::normalize(glm::cross(glm::vec3(0.f, -1.f, 0.f), w))};
	const glm::vec3 v{glm::normalize(glm::cross(w, u))};

	auto viewMatrix = glm::mat4{1.f};
	viewMatrix[0][0] = u.x;
	viewMatrix[1][0] = u.y;
	viewMatrix[2][0] = u.z;
	viewMatrix[0][1] = v.x;
	viewMatrix[1][1] = v.y;
	viewMatrix[2][1] = v.z;
	viewMatrix[0][2] = w.x;
	viewMatrix[1][2] = w.y;
	viewMatrix[2][2] = w.z;
	viewMatrix[3][0] = -glm::dot(u, transform.translation);
	viewMatrix[3][1] = -glm::dot(v, transform.translation);
	viewMatrix[3][2] = -glm::dot(w, transform.translation);

	return viewMatrix;
}

glm::mat4 FestiCamera::getInverseView() const {
	const float c2 = glm::cos(transform.rotation.x);
	const float s2 = glm::sin(transform.rotation.x);
	const float c1 = glm::cos(transform.rotation.y);
	const float s1 = glm::sin(transform.rotation.y);
	const glm::vec3 w = glm::vec3((c2 * s1), (-s2), (c1 * c2));
	const glm::vec3 u{glm::normalize(glm::cross(glm::vec3(0.f, -1.f, 0.f), w))};
	const glm::vec3 v{glm::normalize(glm::cross(w, u))};

	auto inverseViewMatrix = glm::mat4{1.f};
	inverseViewMatrix[0][0] = u.x;
	inverseViewMatrix[0][1] = u.y;
	inverseViewMatrix[0][2] = u.z;
	inverseViewMatrix[1][0] = v.x;
	inverseViewMatrix[1][1] = v.y;
	inverseViewMatrix[1][2] = v.z;
	inverseViewMatrix[2][0] = w.x;
	inverseViewMatrix[2][1] = w.y;
	inverseViewMatrix[2][2] = w.z;
	inverseViewMatrix[3][0] = transform.translation.x;
	inverseViewMatrix[3][1] = transform.translation.y;
	inverseViewMatrix[3][2] = transform.translation.z;

	return inverseViewMatrix;
}

double FestiCamera::mousedx = 0.0;
double FestiCamera::mousedy = 0.0;
VkExtent2D FestiCamera::activeWindowExtent = {0, 0};

void FestiCamera::updateCameraFromKeyPresses(float dt) {
	auto win = festiWindow.getGLFWwindow();

	// Accept Mouse Movement
	if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		festiWindow.inWindow = !festiWindow.inWindow;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if (festiWindow.inWindow) {
		glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		glfwSetCursorPos(win, festiWindow.getExtent().width / 2, festiWindow.getExtent().height / 2);
		activeWindowExtent = festiWindow.getExtent();

		glm::vec3 rotate{-mousedy, -mousedx, 0};
		if (glm::length(rotate) > std::numeric_limits<float>::epsilon()) {
			transform.rotation += lookSpeed * dt * rotate;
		}

	} else {
		glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
		
	// limit pitch values between about +/- 85ish degrees
	transform.rotation.x = glm::clamp(transform.rotation.x, -1.5f, 1.5f);
	transform.rotation.y = glm::mod(transform.rotation.y, glm::two_pi<float>());

	float yaw = transform.rotation.y;
	const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
	const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
	const glm::vec3 upDir{0.f, -1.f, 0.f};

	glm::vec3 moveDir{0.f};
	if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) moveDir -= forwardDir;
	if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) moveDir += forwardDir;
	if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) moveDir -= rightDir;
	if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) moveDir += rightDir;
	if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) moveDir -= upDir;
	if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) moveDir += upDir;

	if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
		transform.translation += moveSpeed * dt * glm::normalize(moveDir);
	}
}

void FestiCamera::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	(void)window;
	(void)xoffset;
    if (yoffset > 0) {
        moveSpeed += 1;
    } else if (yoffset < 0) {
        moveSpeed -= 1;
        if (moveSpeed < 1) moveSpeed = 1;
    }
	moveSpeed = glm::clamp(moveSpeed, 0.0f, 500.0f);
}

void FestiCamera::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	(void)window;
    mousedx = xpos - activeWindowExtent.width / 2;
    mousedy = activeWindowExtent.height / 2 - ypos;
}

float FestiCamera::moveSpeed = 3.f;

}  // namespace festi
