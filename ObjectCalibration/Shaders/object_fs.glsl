#version 430

in vec2 uv;

layout(location = 0) out vec4 color;

uniform sampler2D image;

void main()
{
	color = vec4(0.1 + 0.8 * texture(image, uv).rgb, 1.0);
}
