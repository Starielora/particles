#pragma once

#include "shader.h"
#include "Timer.h"
#include "OpenGLUtils.h"

#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glad/glad.h>

#include <list>
#include <random>

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

class InstancedParticleSystem final
{
	const GLuint VAO, VBO, EBO, instanceVBO, textureId, FBO;
	const Shader squareShader, circleShader;

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

	struct InstanceData
	{
		glm::vec4 color;
		glm::mat4 transformation;
	};

	std::vector<InstanceData> instancesData;

public:
	InstancedParticleSystem(unsigned int pool, unsigned int width, unsigned int height)
		: VAO(gl::genVertexArray())
		, VBO(gl::genBuffer())
		, EBO(gl::genBuffer())
		, instanceVBO(gl::genBuffer())
		, textureId(gl::genTexture(width, height))
		, FBO(gl::genFramebuffer(textureId))
		, circleShader("D:/dev/particle-system/assets/CircleOnQuad_Instanced.vert", "D:/dev/particle-system/assets/CircleOnQuad_Instanced.frag")
		, squareShader("D:/dev/particle-system/assets/Square_Instanced.vert", "D:/dev/particle-system/assets/Square_Instanced.frag")
		, particlesLimit(pool)
	{
		assert(VAO != 0);
		assert(VBO != 0);
		assert(EBO != 0);

		constexpr float VERTICES[] = {
			// positions		
			-1.f, -1.f, 0.0f,
			 1.f, -1.f, 0.0f,
			 1.f,  1.f, 0.0f,
			-1.f,  1.f, 0.0f
		};

		constexpr unsigned int INDICES[] = {
			0, 1, 2,  // first Triangle
			2, 3, 0   // second Triangle
		};

		instancesData.resize(pool);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * particlesLimit, nullptr, GL_DYNAMIC_DRAW);

		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (const void*)(0 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (const void*)(1 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (const void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(3);

		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (const void*)(3 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(4);

		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (const void*)(4 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);

		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void draw(glm::mat4 view, glm::mat4 projection, float currentTime)
	{
		auto instanceIndex = std::size_t{ 0 };
		for (auto particle = aliveParticles.begin(); particle != aliveParticles.end(); particle++)
		{
			const auto particleLifetime = currentTime - particle->creationTime;
			particle->isAlive = particleLifetime < particle->totalLifeTime;

			if (particle->isAlive)
			{
				const auto progress = particleLifetime / particle->totalLifeTime;

				// translation like this is a lot faster than using glm::translate on eye matrix
				auto transformation = glm::mat4(glm::vec4{ 1.f, 0.f, 0.f, 0.f },
											    glm::vec4{ 0.f, 1.f, 0.f, 0.f },
											    glm::vec4{ 0.f, 0.f, 1.f, 0.f },
											    glm::vec4{ particle->position, 1.f });

				// TODO slow af, try quaternions
				//transformation = glm::rotate(transformation, zRotation, glm::vec3{ 0.f, 0.f, 1.f });
				transformation = glm::scale(transformation, glm::vec3{ particle->scale, particle->scale, particle->scale });

				auto& instanceData = instancesData[instanceIndex++];
				instanceData.color = glm::lerp(particle->startColor, particle->endColor, progress);
				instanceData.transformation = transformation;

				particle->position += particle->velocity;
				particle->velocity += particle->acceleration;
			}
		}
		aliveParticles.remove_if([](const auto& p) { return !p.isAlive; });

		auto& shader = properties.particleShape == 0 ? squareShader : circleShader;
		auto thickness = properties.shapeThickness;

		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		gl::checkError();

		glClearColor(0.f, 0.f, 0.f, 0.f);
		gl::checkError();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		gl::checkError();

		shader.use();
		shader.setMat4("view", view);
		shader.setMat4("projection", projection);
		if (properties.particleShape == 1) // TODO remove after adding thickness to other shapes
		{
			shader.setFloat("thickness", thickness);
		}

		const auto dataSize = sizeof(InstanceData) * instanceIndex;
		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		gl::checkError();
		glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, instancesData.data());
		gl::checkError();

		glBindVertexArray(VAO);
		gl::checkError();
		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instanceIndex);
		gl::checkError();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		gl::checkError();
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

	void emit(glm::vec3 worldPos, float t)
	{
		// structured binding
		auto&
			[ initialVelocity
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
