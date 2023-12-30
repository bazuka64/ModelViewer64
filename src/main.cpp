#include "config.h"
#include "Shader.h"

int main()
{
	glfwInit();
	GLFWwindow* window = glfwCreateWindow(1280, 720, "", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGL();

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

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shader.program);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}