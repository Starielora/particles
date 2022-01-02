#pragma once

#include "OpenGLUtils.h"

class TexturedQuad final
{
	const GLuint _VAO, _VBO, _EBO;

public:
	TexturedQuad()
		: _VAO(gl::genVertexArray())
		, _VBO(gl::genBuffer())
		, _EBO(gl::genBuffer())
	{
		constexpr float VERTICES[] = {
			-1.f, -1.f, 0.f, 0.f, // bl
			 1.f, -1.f, 1.f, 0.f, // br
			 1.f,  1.f, 1.f, 1.f, // tr
			-1.f,  1.f, 0.f, 1.f  // tl
		};

		constexpr unsigned int INDICES[] = { 0, 1, 2, 2, 3, 0 };

		glBindVertexArray(_VAO);
		gl::checkError();
		glBindBuffer(GL_ARRAY_BUFFER, _VBO);
		gl::checkError();

		glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);
		gl::checkError();

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)(0 * sizeof(float)));
		gl::checkError();
		glEnableVertexAttribArray(0);
		gl::checkError();

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)(2 * sizeof(float)));
		gl::checkError();
		glEnableVertexAttribArray(1);
		gl::checkError();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _EBO);
		gl::checkError();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW);
		gl::checkError();

		glBindVertexArray(0);
	}

	~TexturedQuad()
	{
		glDeleteBuffers(1, &_VBO);
		gl::checkError();
		glDeleteBuffers(1, &_EBO);
		gl::checkError();
		glDeleteVertexArrays(1, &_VAO);
		gl::checkError();
	}

	auto VAO() const { return _VAO; }
};
