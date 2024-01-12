#pragma once
#include "config.h"

class Model
{
public:
	Shader* shader;
	Transform transform;
	virtual ~Model() {} // need
};