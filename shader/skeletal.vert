#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 3) in ivec4 boneIDs;
layout (location = 4) in vec4 weights;

out vec2 fuv;
out vec3 fnormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 FinalTransform[200];

void main()
{
	mat4 skinned = FinalTransform[boneIDs[0]] * weights[0] +
				   FinalTransform[boneIDs[1]] * weights[1] +
				   FinalTransform[boneIDs[2]] * weights[2] +
				   FinalTransform[boneIDs[3]] * weights[3] ;

	gl_Position = projection * view * model * skinned * vec4(position, 1.0);
	fuv = uv;
	fnormal = normal;
}