#pragma once
#include "config.h"

class Camera
{
public:
	glm::vec3 position{ -1000,1000,1000 };
	glm::vec3 target{ 2000,0,-2000 };
	glm::vec3 worldUp{ 0,1,0 };

	glm::mat4 view;
	glm::mat4 projection;

	float fov = 45;
	float aspect = 16. / 9.;
	float znear = 10;
	float zfar = 100000;

	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 up;

	float yaw;
	float pitch;

	float speed = 500;
	float sensitivity = 0.1;
	float scrollSpeed = 10;

	Camera()
	{
		front = glm::normalize(target - position);
		right = glm::normalize(glm::cross(front, worldUp));
		up = glm::normalize(glm::cross(right, front));

		yaw = glm::degrees(atan2(front.z, front.x));
		pitch = glm::degrees(asin(front.y));
	}

	void UpdatePosition(GLFWwindow* window, float dt)
	{
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			position += front * speed * dt;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			position -= front * speed * dt;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			position += right * speed * dt;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			position -= right * speed * dt;
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			position += up * speed * dt;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			position -= up * speed * dt;

		target = position + front;
	}

	void UpdateRotation(glm::vec2 deltaPos)
	{
		yaw += deltaPos.x * sensitivity;
		pitch -= deltaPos.y * sensitivity;
		pitch = glm::clamp<float>(pitch, -89, 89);

		front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		front.y = sin(glm::radians(pitch));
		front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));

		right = glm::normalize(glm::cross(front, worldUp));
		up = glm::normalize(glm::cross(right, front));

		target = position + front;
	}

	void UpdateScroll(float yoffset)
	{
		position += front * yoffset * scrollSpeed;
		target = position + front;
	}

	void UpdateMatrix()
	{
		view = glm::lookAt(position, target, worldUp);
		projection = glm::perspective(glm::radians(fov), aspect, znear, zfar);
	}
};