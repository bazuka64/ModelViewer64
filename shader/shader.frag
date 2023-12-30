#version 460

in vec2 fuv;

out vec4 color;

uniform sampler2D tex0;

void main()
{
	color = texture(tex0, fuv);
}