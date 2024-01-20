#version 460

in vec3 fnormal;
in vec3 fcolor;
in vec2 fuv;

out vec4 color;

uniform sampler2D tex0;

vec3 lightDir = normalize(vec3(1));

void main()
{
	float light = .5 + .5 * clamp(dot(fnormal, lightDir), 0, 1);

	vec4 texColor = texture(tex0,fuv);
	vec3 mainColor = mix(fcolor, texColor.rgb, texColor.a);
	color = vec4(mainColor * light, 1);
}