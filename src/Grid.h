#pragma once
#include "config.h"
#include "MMDModel.h"

extern std::vector<Model*> models;
extern Camera* camera;

class Grid
{
public:
	int GridSize = 400;
	int GridNum = 10;
	int SelectedGrid = -1;
	int DestinationGrid = -1;
	glm::vec3 offset;
	std::vector<Model*> modelMap{ (unsigned int)(GridNum * GridNum) };

	void Save(const char* path)
	{
		std::ofstream file(path);

		for (int i = 0; i < modelMap.size(); i++)
		{
			Model* model = modelMap[i];
			if (model)
			{
				file << i << " " << model->path << std::endl;
			}
		}
	}

	void ReallocModel()
	{
		Model* model = modelMap[SelectedGrid];
		int gridID = PositionToGridID(model->transform.position + offset);
		DestinationGrid = -1;

		if (gridID != -1)
		{
			// swap model
			if (modelMap[gridID])
			{
				Model* destModel = modelMap[gridID];
				modelMap[SelectedGrid] = destModel;

				destModel->transform.position = GridIDToPosition(SelectedGrid);
				destModel->UpdateTransform();
			}
			else
				modelMap[SelectedGrid] = NULL;

			modelMap[gridID] = model;
			SelectedGrid = gridID;

			model->transform.position = GridIDToPosition(gridID);
			model->UpdateTransform();
		}
		else
		{
			model->transform.position = GridIDToPosition(SelectedGrid);
			model->UpdateTransform();
		}
	}

	void AddModelWithID(Model* model, int gridID)
	{
		modelMap[gridID] = model;
		models.push_back(model);

		model->transform.position = GridIDToPosition(gridID);

		if (model->MaxSize)
		{
			float scale = GridSize / model->MaxSize;
			model->transform.scale = glm::vec3(scale);
		}
		else
		{
			model->transform.scale = glm::vec3(20);
		}

		model->UpdateTransform();
	}

	void AddModel(Model* model, int gridID)
	{
		if (gridID != -1)
			AddModelWithID(model, gridID);
		else
		{
			for (int i = 0; i < modelMap.size(); i++)
			{
				if (modelMap[i] == NULL)
				{
					AddModelWithID(model, i);
					break;
				}
			}
		}
	}

	glm::vec3 GridIDToPosition(int gridID)
	{
		glm::vec3 position;
		position.x = gridID % GridNum * GridSize + GridSize / 2;
		position.y = 0;
		position.z = -(gridID / GridNum * GridSize + GridSize / 2);
		return position;
	}

	int PositionToGridID(glm::vec3 position)
	{
		int gridX = glm::floor(position.x / GridSize);
		int gridZ = glm::floor(- position.z / GridSize);
		if (gridX >= 0 && gridX < GridNum && gridZ >= 0 && gridZ < GridNum)
			return gridX + gridZ * GridNum;
		else
			return -1;
	}

	void Draw()
	{
		glDisable(GL_DEPTH_TEST);

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

		for(int i=0;i<modelMap.size();i++)
		{
			if (modelMap[i])
			{
				DrawColor(i, glm::vec3(0.3, 0.3, 0.3));
			}
		}

		if (SelectedGrid != -1)
			DrawColor(SelectedGrid, glm::vec3(1,1,0));

		if (DestinationGrid != -1 && DestinationGrid != SelectedGrid)
			DrawColor(DestinationGrid, glm::vec3(1, 0.5, 0));

		glEnable(GL_DEPTH_TEST);
	}

	void DrawColor(int gridID, const glm::vec3& color)
	{
		glColor3fv((float*)&color);
		glLineWidth(5);
		glBegin(GL_LINE_LOOP);

		glm::vec3 pos(gridID % GridNum, 0, -gridID / GridNum);
		pos *= GridSize;
		glVertex3f(pos.x, pos.y, pos.z);
		glVertex3f(pos.x + GridSize, 0, pos.z);
		glVertex3f(pos.x + GridSize, 0, pos.z - GridSize);
		glVertex3f(pos.x, 0, pos.z - GridSize);

		glEnd();
	}
};