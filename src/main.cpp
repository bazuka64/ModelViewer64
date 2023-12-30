#include "config.h"

int main()
{
	glfwInit();
	GLFWwindow* window = glfwCreateWindow(1280, 720, "", NULL, NULL);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}