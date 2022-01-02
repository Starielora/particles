#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 instanceColor;
layout (location = 2) in mat4 instanceModel;

uniform mat4 view;
uniform mat4 projection;

out vec4 particleColor;
out vec3 localPosition;

void main()
{
	gl_Position = projection * view * instanceModel * vec4(aPos, 1.0f);
	particleColor = instanceColor;
	localPosition = aPos;
}
