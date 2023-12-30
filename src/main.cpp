#include "config.h"
#include "Shader.h"
#include "Camera.h"

Camera camera;
glm::vec2 cursorPos;

void WindowSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	if (width != 0 && height != 0)
		camera.aspect = width / (float)height;
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	glm::vec2 pos(xpos, ypos);
	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
	{
		glm::vec2 deltaPos = pos - cursorPos;
		camera.UpdateRotation(deltaPos);
	}
	cursorPos = pos;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.UpdateScroll(yoffset);
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 4);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	glfwSetWindowSizeCallback(window, WindowSizeCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetScrollCallback(window, ScrollCallback);

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	glClearColor(.1, .1, .1, 1.);

	Shader shader("shader/shader.vert", "shader/shader.frag");

	float vertices[]{
		0,1,0,
		-1,-1,0,
		1,-1,0,
	};
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

	float prevTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		float time = glfwGetTime();
		float dt = time - prevTime;
		prevTime = time;

		camera.UpdatePosition(window, dt);
		camera.UpdateMatrix();
		shader.SetCameraMatrix(camera);

		glClear(GL_COLOR_BUFFER_BIT);

		float start = glfwGetTime();

		glUseProgram(shader.program);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		float end = glfwGetTime();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
		glfwPollEvents();

		static float count;
		const float interval = 0.2;
		count += dt;
		if (count > interval)
		{
			count -= interval;
			int fps = 1 / dt;
			float elapsed = end - start;
			std::stringstream title;
			title << "FPS: " << fps << "  Elapsed: " << std::fixed << elapsed;
			glfwSetWindowTitle(window, title.str().c_str());
		}
	}
}