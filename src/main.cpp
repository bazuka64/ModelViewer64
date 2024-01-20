/*
	todo
	mmd skinning sdef
	vmd bezier interpolation
*/

#define STB_IMAGE_IMPLEMENTATION
#include "config.h"
#include "Shader.h"
#include "Camera.h"
#include "MMDModel.h"
#include "MMDAnimation.h"
#include "Grid.h"
#include "StaticModel.h"
#include "SkeletalModel.h"
#include "Model.h"
#include <libsm64/libsm64.h>
#include "MarioModel.h"
#include "sm64_audio.h"

Camera* camera;
glm::vec2 cursorPos;
std::vector<Model*> models;
MarioModel* mario;
Shader* mmd_shader;
Shader* static_shader;
Shader* skeletal_shader;
Shader* mario_shader;
Grid* grid;
MMDAnimation* animation;

bool EnableAnimation = true;
bool EnablePhysics = true;
bool DebugDraw = false;
bool DebugAABB = false;
bool Mute = true;

sf::Music music;
bool isMainLoop = false;
bool isDrag;

void LoadFile(std::string path, int gridID);

void ResetAnimation()
{
	for (Model* model : models)
	{
		if (typeid(*model) == typeid(MMDModel))
			((MMDModel*)model)->Reset();
	}
	music.setPlayingOffset(sf::seconds(0));
	if (isMainLoop)
		music.play();
}

void LoadSaveFile(const char* savefile)
{
	for (int i = 0; i < grid->modelMap.size(); i++)
		grid->modelMap[i] = NULL;
	if(models.size())
		delete[] models.data();
	models.clear();

	std::ifstream file(savefile);
	std::string line;
	while (std::getline(file, line))
	{
		int pos = line.find_first_of(" ");
		int gridID = atoi(line.substr(0, pos).c_str());
		std::string path = line.substr(pos + 1);

		LoadFile(path, gridID);
	}
}

glm::vec3 RayCast(GLFWwindow* window)
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	float ndcX = 2 * (xpos / width) - 1;
	float ndcY = 1 - 2 * (ypos / height);
	glm::vec4 clipCoords(ndcX, ndcY, -1, 1);

	glm::vec4 eyeCoords = glm::inverse(camera->projection) * clipCoords;
	eyeCoords.z = -1;
	eyeCoords.w = 0;

	glm::vec3 worldCoords = glm::inverse(camera->view) * eyeCoords;
	glm::vec3 rayDirection = glm::normalize(worldCoords);

	// x,z•½–Ê‚Æ‚ÌŒð“_‚ð’²‚×‚é
	glm::vec3 n(0, 1, 0);
	float t = -glm::dot(camera->position, n) / glm::dot(rayDirection, n);
	glm::vec3 intersectionPoint = camera->position + t * rayDirection;

	return intersectionPoint;
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	glm::vec2 pos(xpos, ypos);

	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
	{
		glm::vec2 deltaPos = pos - cursorPos;
		camera->UpdateRotation(deltaPos);
	}
	else if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL
		&& isDrag)
	{
		Model* model = grid->modelMap[grid->SelectedGrid];
		if (model)
		{
			glm::vec3 intersectionPoint = RayCast(window);

			model->transform.position = intersectionPoint - grid->offset;
			model->UpdateTransform();

			grid->DestinationGrid = grid->PositionToGridID(intersectionPoint);
		}
	}

	cursorPos = pos;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
		{
			glm::vec3 intersectionPoint = RayCast(window);

			grid->SelectedGrid = grid->PositionToGridID(intersectionPoint);

			if (grid->SelectedGrid != -1 &&
				grid->modelMap[grid->SelectedGrid] != NULL)
			{
				isDrag = true;
				grid->offset = intersectionPoint - grid->modelMap[grid->SelectedGrid]->transform.position;
			}
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE
		&& glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
	{
		if (isDrag)
		{
			grid->ReallocModel();
			isDrag = false;
		}
	}
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
	else if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		if (isDrag)return;
		if (grid->SelectedGrid == -1)return;
		Model* model = grid->modelMap[grid->SelectedGrid];
		if (!model)return;

		for (auto iter = models.begin(); iter != models.end(); iter++)
		{
			if (model == *iter)
			{
				grid->modelMap[grid->SelectedGrid] = NULL;
				models.erase(iter);
				delete model;
				break;
			}
		}
	}
}

// gridID auto : -1
void LoadFile(std::string path, int gridID)
{
	int dotPos = path.find_last_of(".");
	std::string ext = path.substr(dotPos + 1);
	int slashPos = path.find_last_of("/\\");
	std::string filename = path.substr(slashPos + 1);

	if (stricmp(ext.c_str(), "pmx") == 0)
	{
		MMDModel* model = new MMDModel(path, mmd_shader);
		grid->AddModel(model, gridID);
		if (animation)
		{
			model->animation = animation;
		}
	}
	else if (stricmp(ext.c_str(), "vmd") == 0)
	{
		animation = new MMDAnimation(path.c_str());
		for (Model* model : models)
		{
			if (typeid(*model) == typeid(MMDModel))
			{
				MMDModel* mmdModel = (MMDModel*)model;
				mmdModel->animation = animation;
			}
		}
		EnableAnimation = true;
		ResetAnimation();
	}
	else if (stricmp(ext.c_str(), "mp3") == 0 ||
		stricmp(ext.c_str(), "wav") == 0)
	{

		if (!music.openFromFile(path)) throw;

		music.setVolume(Mute ? 0 : 100);

		ResetAnimation();
	}
	else if (stricmp(ext.c_str(), "obj") == 0)
	{
		StaticModel* model = new StaticModel(path, static_shader);
		grid->AddModel(model, gridID);
	}
	else if (stricmp(ext.c_str(), "dae") == 0 ||
		stricmp(ext.c_str(), "fbx") == 0)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, 0);
		if (!scene)
		{
			print(importer.GetErrorString());
			throw;
		}
		Model* model = NULL;
		if (scene->HasAnimations())
			model = new SkeletalModel(path, skeletal_shader);
		else
			model = new StaticModel(path, static_shader);
		grid->AddModel(model, gridID);
	}
	else if (stricmp(filename.c_str(), "savefile.txt") == 0)
	{
		LoadSaveFile(path.c_str());
	}
	else if (stricmp(ext.c_str(), "z64") == 0)
	{
		static bool first = true;
		if (!first)return;
		first = false;

		// sm64 rom
		FILE* f = fopen(path.c_str(), "rb");
		if (!f)throw;
		fseek(f, 0, SEEK_END);
		size_t length = ftell(f);
		rewind(f);
		uint8_t* rom = (uint8_t*)malloc(length);
		fread(rom, 1, length, f);
		fclose(f);

		uint8_t* texture = (uint8_t*)malloc(4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT);

		sm64_global_init(rom, texture);

		sm64_audio_init(rom);
		audio_init();

		free(rom);

		int size = 8000;
		SM64Surface surfaces[]{
			{0,0,0,{{size,0,size},{size,0,-size},{-size,0,size}}},
			{0,0,0,{{-size,0,-size},{-size,0,size},{size,0,-size}}},
		};
		sm64_static_surfaces_load(surfaces, sizeof(surfaces) / sizeof(surfaces[0]));

		mario = new MarioModel(path, mario_shader, texture);

		grid->AddModel(mario, -1);
	}
}

void DropCallback(GLFWwindow* window, int path_count, const char* paths[])
{
	oguna::EncodingConverter conv;

	for (int i = 0; i < path_count; i++)
	{
		std::string path;
		conv.Utf8ToCp932(paths[i], strlen(paths[i]), &path);

		LoadFile(path, -1);
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

	// window position center
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
	camera->aspect = width / (float)height;

	mmd_shader = new Shader("../../shader/mmd.vert", "../../shader/mmd.frag");
	static_shader = new Shader("../../shader/static.vert", "../../shader/assimp.frag");
	skeletal_shader = new Shader("../../shader/skeletal.vert", "../../shader/assimp.frag");
	mario_shader = new Shader("../../shader/mario.vert", "../../shader/mario.frag");

	const char* paths[]{
		"../../res/meirin/meirin.pmx",
		"../../res/mima/mima.pmx",
		"../../res/zettai_zetsumei.vmd",
		"../../res/zettai_zetsumei.mp3",
		"../../res/Mega Man X/model.obj",
		"../../res/3DS - Pokedex 3D Pro - 157 Typhlosion/anim.dae",
		//"C:/Users/pentan/Desktop/baserom.us.z64",
	};
	DropCallback(window, std::size(paths), paths);

	music.play();
	isMainLoop = true;

	float prevTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		float time = glfwGetTime();
		float dt = time - prevTime;
		prevTime = time;

		camera->UpdatePosition(window, dt);
		camera->UpdateMatrix();

		mmd_shader->SetCameraMatrix(camera);
		static_shader->SetCameraMatrix(camera);
		skeletal_shader->SetCameraMatrix(camera);
		mario_shader->SetCameraMatrix(camera);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float start = glfwGetTime();

		grid->Draw();

		for (Model* model : models)
		{
			if (typeid(*model) == typeid(MMDModel))
				((MMDModel*)model)->Draw(dt, EnableAnimation, EnablePhysics, DebugDraw);
			else if (typeid(*model) == typeid(StaticModel))
				((StaticModel*)model)->Draw();
			else if (typeid(*model) == typeid(SkeletalModel))
				((SkeletalModel*)model)->Draw(dt);
		}

		if (mario)
		{
			mario->Update(dt, window);
			mario->Draw();
		}

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
		ImGui::Checkbox("DebugAABB", &DebugAABB);
		if (ImGui::Checkbox("Mute", &Mute))
		{
			if (Mute)
				music.setVolume(0);
			else
				music.setVolume(100);
		}
		ImGui::Text("Press F: Delete Model");
		ImGui::Text("Drop sm64usROM(*.z64): Create Mario");
		ImGui::SliderFloat("Camera Speed", &camera->speed, 1, 1000, "%0.f");
		if (ImGui::Button("Save"))
		{
			grid->Save("../../res/savefile.txt");
		}
		if (ImGui::Button("Reset Animation"))
		{
			ResetAnimation();
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