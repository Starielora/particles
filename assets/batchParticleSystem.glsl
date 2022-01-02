#type vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 view;
uniform mat4 projection;

out vec4 particleColor;

void main()
{
	gl_Position = projection * view * vec4(aPos, 1.0f);
	particleColor = aColor;
}

#type fragment
#version 330 core
out vec4 FragColor;

in vec4 particleColor;

void main()
{
    FragColor = particleColor;
}
