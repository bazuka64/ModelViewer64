#pragma once
#include "config.h"
#include "AABB.h"

class Model
{
public:
	Shader* shader;
	Transform transform;
	float MaxSize;
	std::string path;
	glm::vec3 localMin;
	glm::vec3 localMax;
	AABB aabb;

	Model(std::string path, Shader* shader):path(path),shader(shader){}

	virtual void UpdateTransform()
	{
		transform.UpdateMatrix();
		aabb.min = transform.mat * glm::vec4(localMin, 1);
		aabb.max = transform.mat * glm::vec4(localMax, 1);
	}

	void DrawAABB()
	{
		extern Camera* camera;
		extern bool DebugAABB;
		if (!DebugAABB)return;

		glDisable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf((float*)&camera->projection);

		glm::mat4 ModelView = camera->view;
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((float*)&ModelView);

		glColor3f(0, 1, 1);
		glLineWidth(1);

		glm::vec3& min = aabb.min;
		glm::vec3& max = aabb.max;

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBegin(GL_QUADS);

		// Front face
		glVertex3f(min.x, min.y, min.z);
		glVertex3f(max.x, min.y, min.z);
		glVertex3f(max.x, max.y, min.z);
		glVertex3f(min.x, max.y, min.z);

		// Back face
		glVertex3f(min.x, min.y, max.z);
		glVertex3f(max.x, min.y, max.z);
		glVertex3f(max.x, max.y, max.z);
		glVertex3f(min.x, max.y, max.z);

		// Top face
		glVertex3f(min.x, max.y, min.z);
		glVertex3f(max.x, max.y, min.z);
		glVertex3f(max.x, max.y, max.z);
		glVertex3f(min.x, max.y, max.z);

		// Bottom face
		glVertex3f(min.x, min.y, min.z);
		glVertex3f(max.x, min.y, min.z);
		glVertex3f(max.x, min.y, max.z);
		glVertex3f(min.x, min.y, max.z);

		// Left face
		glVertex3f(min.x, min.y, min.z);
		glVertex3f(min.x, max.y, min.z);
		glVertex3f(min.x, max.y, max.z);
		glVertex3f(min.x, min.y, max.z);

		// Right face
		glVertex3f(max.x, min.y, min.z);
		glVertex3f(max.x, max.y, min.z);
		glVertex3f(max.x, max.y, max.z);
		glVertex3f(max.x, min.y, max.z);

		glEnd();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glEnable(GL_DEPTH_TEST);
	}

	virtual ~Model() {} // need
};