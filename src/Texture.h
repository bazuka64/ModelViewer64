#pragma once
#include "config.h"

class Texture
{
public:
	GLuint id;

	void Load(std::string path)
	{
		int x, y, comp;
		unsigned char* data = stbi_load(path.c_str(), &x, &y, &comp, STBI_rgb_alpha);
		if (!data)return;

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);
	}
};