#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in ivec4 boneIDs;
layout (location = 3) in vec4 weights;
layout (location = 4) in vec3 morphPos;

out vec2 fuv;

uniform mat4 view;
uniform mat4 projection;

uniform BoneMatrix
{
	mat4 FinalTransform[500];
};

void main()
{
	mat4 skinned = FinalTransform[boneIDs[0]] * weights[0] +
				   FinalTransform[boneIDs[1]] * weights[1] +
				   FinalTransform[boneIDs[2]] * weights[2] +
				   FinalTransform[boneIDs[3]] * weights[3] ;

	gl_Position = projection * view * skinned * vec4(position + morphPos, 1.0);
	fuv = uv;
}