#pragma once
#include "config.h"

class Model
{
public:
	Shader* shader;
	Transform transform;
	float MaxSize;

	virtual ~Model() {} // need
};