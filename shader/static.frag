#version 460

in vec2 fuv;
in vec3 fnormal;

out vec4 color;

uniform sampler2D tex0;

vec3 lightDir = normalize(vec3(1,1,1));
float ambient = 0.5;

void main()
{
	float diffuse = abs(dot(fnormal, lightDir));
	float light = ambient + (1 - ambient) * diffuse;

	vec4 texColor = texture(tex0, fuv);
	color = vec4(texColor.rgb * light, texColor.a);
}