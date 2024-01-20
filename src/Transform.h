#pragma once
#include "config.h"

class Transform
{
public:
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale{ 1 };
	glm::mat4 mat{ 1 };

	void UpdateMatrix()
	{
		glm::mat4 transMat = glm::translate(glm::mat4(1), position);
		glm::vec3 rot = glm::radians(rotation);
		glm::mat4 rotMat = glm::eulerAngleXYZ(rot.x, rot.y, rot.z);
		glm::mat4 scaleMat = glm::scale(glm::mat4(1), scale);
		mat = transMat * rotMat * scaleMat;
	}
};