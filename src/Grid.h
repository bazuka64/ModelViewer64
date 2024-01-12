#pragma once
#include "config.h"
#include "Model.h"

extern std::vector<Model*> models;
extern Camera* camera;

class Grid
{
public:
	int GridSize = 20;
	int GridNum = 10;
	int ModelID;

	void AddModel(Model* model)
	{
		model->transform.position.x = ModelID % GridNum * GridSize + GridSize / 2;
		model->transform.position.z = -(ModelID / GridNum * GridSize + GridSize / 2);
		model->transform.UpdateMatrix();
		models.push_back(model);

		ModelID++;
	}

	void Draw()
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((float*)&camera->view);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf((float*)&camera->projection);

		glColor3f(1, 1, 1);
		glLineWidth(1);
		glBegin(GL_LINES);

		for (int i = 0; i < GridNum + 1; i++)
		{
			glVertex3f(0, 0, -i * GridSize);
			glVertex3f(GridNum * GridSize, 0, -i * GridSize);
		}

		for (int i = 0; i < GridNum + 1; i++)
		{
			glVertex3f(i * GridSize, 0, 0);
			glVertex3f(i * GridSize, 0, -GridNum * GridSize);
		}

		glEnd();
	}
};