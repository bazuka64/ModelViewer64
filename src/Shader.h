#pragma once
#include "config.h"

class Shader
{
public:
	GLuint program;
	std::map<std::string, GLint> UniformLocations;

	Shader(const char* vertPath, const char* fragPath)
	{
		GLuint vertex = CompileShader(vertPath, GL_VERTEX_SHADER);
		GLuint fragment = CompileShader(fragPath, GL_FRAGMENT_SHADER);

		program = glCreateProgram();
		glAttachShader(program, vertex);
		glAttachShader(program, fragment);
		glLinkProgram(program);

		int length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		if (length > 0)
		{
			char* infoLog = new char[length];
			glGetProgramInfoLog(program, length, 0, infoLog);
			print(infoLog);
			throw;
		}

		int num;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &num);
		for (int i = 0; i < num; i++)
		{
			char name[256];
			GLsizei length;
			GLint size;
			GLenum type;

			glGetActiveUniform(program, i, sizeof(name), &length, &size, &type, name);
			GLint location = glGetUniformLocation(program, name);
			UniformLocations[name] = location;
		}
	}

private:
	GLuint CompileShader(const char* path, int type)
	{
		std::ifstream file;
		std::stringstream ss;
		std::string str;
		const char* c_str;

		file.open(path);
		if (file.fail())throw;
		ss << file.rdbuf();
		str = ss.str();
		c_str = str.c_str();

		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &c_str, 0);
		glCompileShader(shader);

		int length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		if (length > 0)
		{
			char* infoLog = new char[length];
			glGetShaderInfoLog(shader, length, 0, infoLog);
			print(infoLog);
			throw;
		}

		return shader;
	}
};