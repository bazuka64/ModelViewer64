#pragma once
#include "config.h"
#include "MMDModel.h"

extern std::vector<Model*> models;
extern Camera* camera;

class Grid
{
public:
	int GridSize = 20;
	int GridNum = 10;
	int SelectedGrid = -1;
	std::vector<Model*> modelMap{ (unsigned int)(GridNum * GridNum) };

	void AddModel(Model* model)
	{
		for (int i = 0; i < modelMap.size(); i++)
		{
			if (modelMap[i] == NULL)
			{
				int GridID = i;
				modelMap[i] = model;
				model->GridID = GridID;
				models.push_back(model);

				model->transform.position.x = GridID % GridNum * GridSize + GridSize / 2;
				model->transform.position.z = -(GridID / GridNum * GridSize + GridSize / 2);

				if (model->MaxSize)
				{
					float scale = GridSize / model->MaxSize;
					model->transform.scale = glm::vec3(scale);
				}

				model->transform.UpdateMatrix();

				break;
			}
		}
	}

	void Select(glm::vec3 point)
	{
		int gridX = glm::floor(point.x / GridSize);
		int gridZ = glm::floor(- point.z / GridSize);
		if (gridX >= 0 && gridX < GridNum && gridZ >= 0 && gridZ < GridNum)
			SelectedGrid = gridX + gridZ * GridNum;
		else
			SelectedGrid = -1;
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

		if (SelectedGrid != -1)
		{
			glColor3f(1, 1, 0);
			glLineWidth(5);
			glBegin(GL_LINE_LOOP);

			glm::vec3 pos(SelectedGrid % GridNum, 0, -SelectedGrid / GridNum);
			pos *= GridSize;
			glVertex3f(pos.x, pos.y, pos.z);
			glVertex3f(pos.x + GridSize, 0, pos.z);
			glVertex3f(pos.x + GridSize, 0, pos.z - GridSize);
			glVertex3f(pos.x, 0, pos.z - GridSize);

			glEnd();
		}
	}
};