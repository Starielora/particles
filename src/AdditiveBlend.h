#pragma once

#include "Shader.h"
#include "OpenGLUtils.h"
#include "TexturedQuad.h"

class AdditiveBlend final
{
	const GLuint textureId;
	const GLuint FBO, quadVAO; // TODO quadVAO might not exist
	const Shader shader;
	float _factor = 2.f;

public:
	AdditiveBlend(unsigned int width, unsigned int height, const TexturedQuad& quad)
		: textureId(gl::genTexture(width, height))
		, FBO(gl::genFramebuffer(textureId))
		, quadVAO(quad.VAO())
		, shader("additiveBlend.glsl")
	{
		shader.use();
		shader.setInt("scene", 0);
		shader.setInt("bloomBlur", 1);
	}

	~AdditiveBlend()
	{
		glDeleteFramebuffers(1, &FBO); gl::checkError();
		glDeleteTextures(1, &textureId); gl::checkError();
		glDeleteVertexArrays(1, &quadVAO); gl::checkError();
	}

	auto texture() { return textureId; }
	auto& factor() { return _factor; }

	void draw(GLuint texture1, GLuint texture2)
	{
		shader.use();
		shader.setFloat("factor", _factor);

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		gl::checkError();

		glActiveTexture(GL_TEXTURE0);
		gl::checkError();
		glBindTexture(GL_TEXTURE_2D, texture1);
		gl::checkError();
		glActiveTexture(GL_TEXTURE1);
		gl::checkError();
		glBindTexture(GL_TEXTURE_2D, texture2);
		gl::checkError();

		glBindVertexArray(quadVAO);
		gl::checkError();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		gl::checkError();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		gl::checkError();
		//glUseProgram(0);
		//gl::checkError();
	}

	void resize(unsigned int width, unsigned int height)
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
		gl::checkError();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		gl::checkError();
		glBindTexture(GL_TEXTURE_2D, textureId);
		gl::checkError();
	}
};
