#pragma once

#include "OpenGLUtils.h"
#include "Shader.h"
#include "Timer.h"

#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glad/glad.h>

#include <vector>
#include <list>
#include <array>
#include <random>
#include <iostream>

namespace rng
{
	auto Float()
	{
		static std::random_device rd{};
		static auto seed = std::mt19937{ rd() };
		static auto distribution = std::uniform_real_distribution<float>(-0.002f, 0.002f);

		return distribution(seed);
	}
}

class BatchParticleSystem final
{
	struct Vertex
	{
		glm::vec3 worldPosition;
		glm::vec4 color;
	};

	struct Particle
	{
		glm::vec4 startColor;
		glm::vec4 endColor;
		glm::vec3 position;
		glm::vec3 velocity;
		glm::vec3 acceleration;
		float creationTime;
		float totalLifeTime;
		float rotationSpeed;
		float scale;
		bool isAlive = false;
	};

	const GLuint VAO, VBO, EBO, textureId, FBO;
	const Shader shader;

	std::list<Particle> aliveParticles;

	struct
	{
		glm::vec3 initialVelocity = { 0.f, 0.f, 0.f };
		glm::vec3 acceleration = { 0.f, 0.f, 0.f };
		glm::vec4 startColor = { 0.f, 0.5f, 1.f, 1.f };
		glm::vec4 endColor = { 1.f, 0.f, 0.f, 1.f };
		int totalLifetimeSeconds = 5;
		int spawnCount = 50;
		float scale = 0.0025f;
		int particleShape = 1; // 0 - square, 1 - circle // TODO enum or sth (fast impl for imgui exposure)...
		float shapeThickness = 0.8f;
		bool randomVelocity = true;
		bool randomAcceleration = false;
	} properties;

	const std::size_t particlesLimit;

	std::vector<Vertex> vertices;

public:
	BatchParticleSystem(unsigned int poolCount, unsigned int width, unsigned int height)
		: VAO(gl::genVertexArray())
		, VBO(gl::genBuffer())
		, EBO(gl::genBuffer())
		, textureId(gl::genTexture(width, height))
		, FBO(gl::genFramebuffer(textureId))
		, shader("batchParticleSystem.glsl")
		, particlesLimit(poolCount)
	{
		assert(VAO != 0);
		assert(VBO != 0);
		assert(EBO != 0);

		const auto MAX_QUAD_COUNT = poolCount;
		const auto MAX_VERTICES_COUNT = MAX_QUAD_COUNT * 4;
		const auto MAX_INDICES_COUNT = MAX_QUAD_COUNT * 6;

		vertices.resize(poolCount * 4);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * MAX_VERTICES_COUNT, nullptr, GL_DYNAMIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, worldPosition));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, color));
		glEnableVertexAttribArray(1);

		auto indices = std::vector<unsigned int>(MAX_INDICES_COUNT);
		for (auto i = 0u, offset = 0u; i < MAX_INDICES_COUNT; i+=6, offset+=4)
		{
			indices[0 + i] = 0 + offset;
			indices[1 + i] = 1 + offset;
			indices[2 + i] = 2 + offset;

			indices[3 + i] = 2 + offset;
			indices[4 + i] = 3 + offset;
			indices[5 + i] = 0 + offset;
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);
	}

	auto& startColor() { return properties.startColor; }
	auto& endColor() { return properties.endColor; }
	auto& totalLifetimeSeconds() { return properties.totalLifetimeSeconds; }
	auto& spawnCount() { return properties.spawnCount; }
	auto& scale() { return properties.scale; }
	auto aliveParticlesCount() { return aliveParticles.size(); }
	auto& particleShape() { return properties.particleShape; }
	auto& shapeThickness() { return properties.shapeThickness; }
	auto& initialVelocity() { return properties.initialVelocity; }
	auto& acceleration() { return properties.acceleration; }
	auto& randomVelocity() { return properties.randomVelocity; }
	auto& randomAcceleration() { return properties.randomAcceleration; }

	auto texture() { return textureId; }


	// TODO this seems to be slower than fillQuads; read doc for emplace_back
	void addQuads(glm::vec3 translation, float zRotation, float scale, glm::vec4 color, std::vector<Vertex>& v)
	{
		constexpr auto bottomLeft = glm::vec4{ -0.5f, -0.5f, 0.0f, 1.f };
		constexpr auto bottomRight = glm::vec4{ 0.5f, -0.5f, 0.0f, 1.f };
		constexpr auto topRight = glm::vec4{ 0.5f,  0.5f, 0.0f, 1.f };
		constexpr auto topLeft = glm::vec4{ -0.5f,  0.5f, 0.0f, 1.f };

		auto transformation = glm::mat4(1.0f);
		transformation = glm::translate(transformation, translation);
		//transformation = glm::rotate(transformation, zRotation, glm::vec3{ 0.f, 0.f, 1.f });
		transformation = glm::scale(transformation, glm::vec3{ scale, scale, scale });

		v.emplace_back(Vertex{ glm::vec3{transformation * bottomLeft}, color });
		v.emplace_back(Vertex{ glm::vec3{transformation * bottomRight}, color });
		v.emplace_back(Vertex{ glm::vec3{transformation * topRight}, color });
		v.emplace_back(Vertex{ glm::vec3{transformation * topLeft}, color });
	}

	void fillQuad(glm::vec3 translation, float zRotation, float scale, glm::vec4 color, Vertex& vBottomLeft, Vertex& vBottomRight, Vertex& vTopRight, Vertex& vTopLeft)
	{
		constexpr auto bottomLeft = glm::vec4{ -0.5f, -0.5f, 0.0f, 1.f };
		constexpr auto bottomRight = glm::vec4{ 0.5f, -0.5f, 0.0f, 1.f };
		constexpr auto topRight = glm::vec4{ 0.5f,  0.5f, 0.0f, 1.f };
		constexpr auto topLeft = glm::vec4{ -0.5f,  0.5f, 0.0f, 1.f };
		constexpr auto eye = glm::mat4(1.0f);

		// translation like this is a lot faster than using glm::translate on eye matrix
		auto transformation = glm::mat4(glm::vec4{ scale, 0.f, 0.f, 0.f },
										glm::vec4{ 0.f, scale, 0.f, 0.f },
										glm::vec4{ 0.f, 0.f, scale, 0.f },
										glm::vec4{ translation,   1.f });

		// TODO slow af, try quaternions
		//transformation = glm::rotate(transformation, zRotation, glm::vec3{ 0.f, 0.f, 1.f });
		//transformation = glm::scale(transformation, glm::vec3{ scale, scale, scale });

		vBottomLeft.worldPosition = glm::vec3{ transformation * bottomLeft };
		vBottomRight.worldPosition = glm::vec3{ transformation * bottomRight };
		vTopRight.worldPosition = glm::vec3{ transformation * topRight };
		vTopLeft.worldPosition = glm::vec3{ transformation * topLeft };

		vBottomLeft.color = vBottomRight.color = vTopRight.color = vTopLeft.color = color;
	}

	void draw(glm::mat4 view, glm::mat4 projection, float currentTime)
	{
		if (aliveParticles.empty())
			return;

		const auto totalLifetimeSeconds = properties.totalLifetimeSeconds;

		auto vertexIndex = 0u;
		for (auto particleIt = aliveParticles.rbegin(); particleIt != aliveParticles.rend(); particleIt++)
		{
			auto& particle = *particleIt;
			const auto particleLifetime = currentTime - particle.creationTime;
			if (particleLifetime > totalLifetimeSeconds)
			{
				particle.isAlive = false;
			}

			if (particle.isAlive)
			{
				const auto progress = particleLifetime / float(totalLifetimeSeconds);
				const auto color = glm::lerp(particle.startColor, particle.endColor, progress);
				//addQuads(particle.position, particle.rotationSpeed * progress, particle.scale, color, vertices);
				auto& bottomLeft = vertices[vertexIndex++];
				auto& bottomRight = vertices[vertexIndex++];
				auto& topRight = vertices[vertexIndex++];
				auto& topLeft = vertices[vertexIndex++];
				fillQuad(particle.position, particle.rotationSpeed * progress, particle.scale, color, bottomLeft, bottomRight, topRight, topLeft);
				particle.position += particle.velocity;
				particle.velocity += particle.acceleration;
			}
		}

		aliveParticles.remove_if([](const auto& p) { return !p.isAlive; });

		shader.use();
		shader.setMat4("view", view);
		shader.setMat4("projection", projection);

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		gl::checkError();

		glClearColor(0.f, 0.f, 0.f, 0.f);
		gl::checkError();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		gl::checkError();

		const auto verticesDataSize = sizeof(Vertex) * vertexIndex;
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		gl::checkError();
		glBufferSubData(GL_ARRAY_BUFFER, 0, verticesDataSize, vertices.data());
		gl::checkError();

		const auto indicesCount = (vertexIndex / 4) * 6;
		glBindVertexArray(VAO);
		gl::checkError();
		glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);
		gl::checkError();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		gl::checkError();
	}

	void emit(glm::vec3 worldPos, float t)
	{
		// structured binding
		auto&
			[initialVelocity
			, acceleration
			, startColor
			, endColor
			, totalLifetimeSeconds
			, spawnCount
			, scale
			, shape
			, thickness
			, randomVelocity
			, randomAcceleration
		] = properties;

		for (auto i = 0; i < spawnCount; i++)
		{
			if (aliveParticles.size() >= particlesLimit)
			{
				std::cerr << "All particles are alive." << std::endl;
				return; // cannot emit
			}

			auto particle = Particle{};
			particle.position = worldPos;
			particle.velocity = randomVelocity ? glm::vec3{ rng::Float(), rng::Float(), rng::Float() } : initialVelocity;
			particle.acceleration = randomAcceleration ? glm::vec3{ rng::Float() / 10.f, rng::Float() / 10.f, rng::Float() / 10.f } : acceleration;
			particle.creationTime = t;
			particle.totalLifeTime = totalLifetimeSeconds;
			particle.rotationSpeed = rng::Float() * 10000;
			particle.startColor = startColor;
			particle.endColor = endColor;
			particle.scale = scale;
			particle.isAlive = true;
			aliveParticles.push_back(particle);
		}
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
