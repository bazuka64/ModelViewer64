#pragma once
#include "config.h"

class MarioModel : public Model
{
public:
	GLuint vao;
	GLuint position_buffer;
	GLuint normal_buffer;
	GLuint color_buffer;
	GLuint uv_buffer;
	GLuint textureID;

	int32_t marioId;
	SM64MarioInputs marioInputs;
	SM64MarioState marioState;
	SM64MarioGeometryBuffers marioGeometry;
	float lastGeoPos[9 * SM64_GEO_MAX_TRIANGLES];
	float currGeoPos[9 * SM64_GEO_MAX_TRIANGLES];
	int GamepadID = -1;
	GLFWgamepadstate state;
	float tick;

	MarioModel(std::string path, Shader* shader, uint8_t* texture)
		: Model(path, shader)
	{
		marioId = sm64_mario_create(0, 0, 0);

		marioGeometry.position = (float*)malloc(SM64_GEO_MAX_TRIANGLES * 3 * 3 * sizeof(float));
		marioGeometry.normal = (float*)malloc(SM64_GEO_MAX_TRIANGLES * 3 * 3 * sizeof(float));
		marioGeometry.color = (float*)malloc(SM64_GEO_MAX_TRIANGLES * 3 * 3 * sizeof(float));
		marioGeometry.uv = (float*)malloc(SM64_GEO_MAX_TRIANGLES * 3 * 2 * sizeof(float));
		marioGeometry.numTrianglesUsed = 0;

		for (int i = 0; i < GLFW_JOYSTICK_LAST; i++)
		{
			if (glfwJoystickPresent(i))
				if (glfwJoystickIsGamepad(i))
					GamepadID = i;
		}

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &position_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
		glBufferData(GL_ARRAY_BUFFER, SM64_GEO_MAX_TRIANGLES * 3 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

		glGenBuffers(1, &normal_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		glBufferData(GL_ARRAY_BUFFER, SM64_GEO_MAX_TRIANGLES * 3 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, 0);

		glGenBuffers(1, &color_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
		glBufferData(GL_ARRAY_BUFFER, SM64_GEO_MAX_TRIANGLES * 3 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, false, 0, 0);

		glGenBuffers(1, &uv_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
		glBufferData(GL_ARRAY_BUFFER, SM64_GEO_MAX_TRIANGLES * 3 * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, false, 0, 0);

		glBindVertexArray(0);

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SM64_TEXTURE_WIDTH, SM64_TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Update(float dt, GLFWwindow* window)
	{
		if (GamepadID != -1)
		{
			// gamepad
			glfwGetGamepadState(GamepadID, &state);

			marioInputs.buttonA = state.buttons[GLFW_GAMEPAD_BUTTON_CROSS];
			marioInputs.buttonB = state.buttons[GLFW_GAMEPAD_BUTTON_SQUARE];
			marioInputs.buttonZ = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];

			marioInputs.stickX = read_axis(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]);
			marioInputs.stickY = read_axis(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
		}
		else
		{
			// keyboard
			marioInputs.buttonA = glfwGetKey(window, GLFW_KEY_C);
			marioInputs.buttonB = glfwGetKey(window, GLFW_KEY_X);
			marioInputs.buttonZ = glfwGetKey(window, GLFW_KEY_Z);

			int x = 0;
			int y = 0;
			x += glfwGetKey(window, GLFW_KEY_RIGHT);
			x -= glfwGetKey(window, GLFW_KEY_LEFT);
			y += glfwGetKey(window, GLFW_KEY_DOWN);
			y -= glfwGetKey(window, GLFW_KEY_UP);
			marioInputs.stickX = x;
			marioInputs.stickY = y;
		}
		
		extern Camera* camera;

		marioInputs.camLookX = camera->front.x;
		marioInputs.camLookZ = camera->front.z;

		tick += dt;
		if (tick >= 1.f / 30)
		{
			tick -= 1.f / 30;

			// collision
			if (marioState.velocity[1] < 0)
			{
				for (Model* model : models)
				{
					AABB& aabb = model->aabb;
					float* pos = marioState.position;
					if (pos[0] >= aabb.min.x &&
						pos[0] <= aabb.max.x &&
						pos[1] >= aabb.min.y &&
						pos[1] <= aabb.max.y &&
						pos[2] >= aabb.min.z &&
						pos[2] <= aabb.max.z)
					{
						sm64_mario_attack(marioId, 0, 0, 0, 0);
						break;
					}
				}
			}

			memcpy(lastGeoPos, currGeoPos, sizeof(currGeoPos));

			sm64_mario_tick(marioId, &marioInputs, &marioState, &marioGeometry);

			memcpy(currGeoPos, marioGeometry.position, sizeof(currGeoPos));
		}

		float factor = tick / (1.f / 30);
		for (int i = 0; i < marioGeometry.numTrianglesUsed * 9; i++)
			marioGeometry.position[i] = lastGeoPos[i] + (currGeoPos[i] - lastGeoPos[i]) * factor;
	}

	void Draw()
	{
		glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
		glBufferData(GL_ARRAY_BUFFER, SM64_GEO_MAX_TRIANGLES * 3 * 3 * sizeof(float), marioGeometry.position, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		glBufferData(GL_ARRAY_BUFFER, SM64_GEO_MAX_TRIANGLES * 3 * 3 * sizeof(float), marioGeometry.normal, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
		glBufferData(GL_ARRAY_BUFFER, SM64_GEO_MAX_TRIANGLES * 3 * 3 * sizeof(float), marioGeometry.color, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
		glBufferData(GL_ARRAY_BUFFER, SM64_GEO_MAX_TRIANGLES * 3 * 2 * sizeof(float), marioGeometry.uv, GL_DYNAMIC_DRAW);

		glUseProgram(shader->program);
		glBindVertexArray(vao);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glDrawArrays(GL_TRIANGLES, 0, marioGeometry.numTrianglesUsed * 3);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);
	}

	void UpdateTransform()
	{
		//Model::UpdateTransform();
		sm64_set_mario_position(marioId,
			transform.position.x,
			transform.position.y,
			transform.position.z
		);
	}

private:
	float read_axis(float val)
	{
		if (val < 0.2f && val > -0.2f)
			return 0.0f;

		return val > 0.0f ? (val - 0.2f) / 0.8f : (val + 0.2f) / 0.8f;
	}
};