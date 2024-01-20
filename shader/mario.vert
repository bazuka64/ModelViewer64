#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 color;
layout (location = 3) in vec2 uv;

out vec3 fnormal;
out vec3 fcolor;
out vec2 fuv;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * vec4(position, 1);

	fnormal = normal;
	fcolor = color;
	fuv = uv;
}