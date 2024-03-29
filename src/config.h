#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stb_image.h>

#include <bullet/btBulletDynamicsCommon.h>

#include <SFML/Audio.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <fstream>
#include <sstream>
#include <iostream>
#define print(x) std::cout << (x) << std::endl
#include <map>
#include <algorithm>
#include <thread>

#include <boost/filesystem.hpp>