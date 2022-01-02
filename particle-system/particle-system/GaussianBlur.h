#pragma once

#include "OpenGLUtils.h"
#include "TexturedQuad.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>

class GaussianBlur final
{
	const GLuint horizontalBlurTexture, verticalBlurTexture;
	const GLuint horizontalFBO, verticalFBO;
	const GLuint quadVAO; // TODO this might not exist
	const Shader shader;
	int _iterations = 9;

public:
	GaussianBlur(unsigned int width, unsigned int height, const TexturedQuad& quad)
		: horizontalBlurTexture(gl::genTexture(width, height))
		, verticalBlurTexture(gl::genTexture(width, height))
		, horizontalFBO(gl::genFramebuffer(horizontalBlurTexture))
		, verticalFBO(gl::genFramebuffer(verticalBlurTexture))
		, quadVAO(quad.VAO())
		, shader("D:/dev/particle-system/assets/blur.glsl")
	{
	}

	auto texture() { return horizontalBlurTexture; }
	auto& iterations() { return _iterations; }

	void draw(GLuint texture)
	{
		shader.use();

		glBindVertexArray(quadVAO);
		gl::checkError();

		glActiveTexture(GL_TEXTURE0);
		gl::checkError();

		// TODO this can be shortened
		glBindFramebuffer(GL_FRAMEBUFFER, horizontalFBO);
		gl::checkError();
		shader.setInt("horizontal", true);
		glBindTexture(GL_TEXTURE_2D, texture);
		gl::checkError();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		gl::checkError();

		glBindFramebuffer(GL_FRAMEBUFFER, verticalFBO);
		gl::checkError();
		shader.setInt("horizontal", false);
		glBindTexture(GL_TEXTURE_2D, horizontalBlurTexture);
		gl::checkError();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		gl::checkError();

		for (auto i = 0u; i < _iterations; ++i)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, horizontalFBO);
			gl::checkError();
			shader.setInt("horizontal", true);
			glBindTexture(GL_TEXTURE_2D, verticalBlurTexture);
			gl::checkError();
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
			gl::checkError();

			glBindFramebuffer(GL_FRAMEBUFFER, verticalFBO);
			gl::checkError();
			shader.setInt("horizontal", false);
			glBindTexture(GL_TEXTURE_2D, horizontalBlurTexture);
			gl::checkError();
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
			gl::checkError();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		gl::checkError();
		glBindVertexArray(0);
		gl::checkError();
	}

	void resize(unsigned int width, unsigned int height)
	{
		glBindTexture(GL_TEXTURE_2D, horizontalBlurTexture);
		gl::checkError();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		gl::checkError();
		glBindTexture(GL_TEXTURE_2D, verticalBlurTexture);
		gl::checkError();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		gl::checkError();
		glBindTexture(GL_TEXTURE_2D, 0);
		gl::checkError();
	}
};
