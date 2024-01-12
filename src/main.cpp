/*
	todo
	mmd skinning sdef
	vmd bezier interpolation
	mmd physics ボーン位置合わせ
	mmd morph 頂点モーフ以外
*/

#define STB_IMAGE_IMPLEMENTATION
#include "config.h"
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Animation.h"
#include "Grid.h"

Camera* camera;
glm::vec2 cursorPos;
std::vector<Model*> models;
Shader* shader;
Grid* grid;
Animation* anim;

bool EnableAnimation = true;
bool EnablePhysics = true;
bool DebugDraw = false;
bool Mute = true;

sf::Music music;

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	glm::vec2 pos(xpos, ypos);
	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
	{
		glm::vec2 deltaPos = pos - cursorPos;
		camera->UpdateRotation(deltaPos);
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
	camera->UpdateScroll(yoffset);
}

void WindowSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	if (width != 0 && height != 0)
		camera->aspect = width / (float)height;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void DropCallback(GLFWwindow* window, int path_count, const char* paths[])
{
	oguna::EncodingConverter conv;

	for (int i = 0; i < path_count; i++)
	{
		std::string path;
		conv.Utf8ToCp932(paths[i], strlen(paths[i]), &path);

		int dotPos = path.find_last_of(".");
		std::string ext = path.substr(dotPos + 1);

		if (stricmp(ext.c_str(), "pmx") == 0)
		{
			Model* model = new Model(path, shader);
			grid->AddModel(model);
			if (anim)
			{
				model->anim = anim;
			}
		}
		else if (stricmp(ext.c_str(), "vmd") == 0)
		{
			anim = new Animation(path.c_str());
			for (Model* model : models)
			{
				model->anim = anim;
				model->Reset();
			}
			EnableAnimation = true;
			music.setPlayingOffset(sf::seconds(0));
		}
		else if (stricmp(ext.c_str(), "mp3") == 0 || 
				 stricmp(ext.c_str(), "wav") == 0)
		{
			
			if (!music.openFromFile(path)) throw;

			music.setVolume(Mute ? 0 : 100);
			music.play();

			for (Model* model : models)
				model->Reset();
		}
	}
}

int main()
{
	int width = 1920;
	int height = 1080;

	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_VISIBLE, false);
	GLFWwindow* window = glfwCreateWindow(width, height, "", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	int xpos = (mode->width - width) / 2;
	int ypos = (mode->height - height) / 2;
	glfwSetWindowPos(window, xpos, ypos);
	glfwShowWindow(window);

	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetWindowSizeCallback(window, WindowSizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetDropCallback(window, DropCallback);

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	glClearColor(.1, .1, .1, 1.);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_CULL_FACE);

	grid = new Grid();

	camera = new Camera();

	shader = new Shader("shader/shader.vert", "shader/shader.frag");

	const char* paths[]{
		"../../res/meirin/meirin.pmx",
		"../../res/mima/mima.pmx",
		"../../res/zettai_zetsumei.vmd",
		"../../res/zettai_zetsumei.mp3",
	};
	DropCallback(window, std::size(paths), paths);

	float prevTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		float time = glfwGetTime();
		float dt = time - prevTime;
		prevTime = time;

		camera->UpdatePosition(window, dt);
		camera->UpdateMatrix();
		shader->SetCameraMatrix(camera);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float start = glfwGetTime();

		grid->Draw();

		for (Model* model : models)
			model->Draw(dt, EnableAnimation, EnablePhysics, DebugDraw);

		float end = glfwGetTime();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (ImGui::Checkbox("Animation", &EnableAnimation))
		{
			if (EnableAnimation)
				music.play();
			else
				music.pause();
		}
		ImGui::Checkbox("Physics", &EnablePhysics);
		ImGui::Checkbox("DebugDraw", &DebugDraw);
		if (ImGui::Checkbox("Mute", &Mute))
		{
			if (Mute)
				music.setVolume(0);
			else
				music.setVolume(100);
		}

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