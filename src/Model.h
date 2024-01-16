#pragma once
#include "config.h"

class Model
{
public:
	Shader* shader;
	Transform transform;
	float MaxSize;
	std::string path;

	Model(std::string path, Shader* shader):path(path),shader(shader){}

	virtual ~Model() {} // need
};