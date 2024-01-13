#pragma once
#include "config.h"

class Model
{
public:
	Shader* shader;
	Transform transform;
	float MaxSize;
	int GridID;

	virtual ~Model() {} // need
};