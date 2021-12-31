#pragma once

#include "shader.h"
#include "Timer.h"

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
	const GLuint VAO, VBO, EBO, instanceVBO;
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


	void glCheckError_(const char* file, int line)
	{
		GLenum errorCode;
		while ((errorCode = glGetError()) != GL_NO_ERROR)
		{
			std::string error;
			switch (errorCode)
			{
			case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
			case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
			case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
			case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
			case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
			case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
			}
			throw std::runtime_error(error + " | " + std::string(file) + " (" + std::to_string(line) + ")");
		}
	}
#define glCheckError() glCheckError_(__FILE__, __LINE__) 

	GLuint genVertexArray()
	{
		auto VAO = GLuint{ 0 };
		glGenVertexArrays(1, &VAO);
		glCheckError();
		return VAO;
	}

	GLuint genBuffer()
	{
		auto buffer = GLuint{ 0 };
		glGenBuffers(1, &buffer);
		glCheckError();
		return buffer;
	}

	struct
	{
		glm::vec4 startColor = { 0.f, 1.f, 0.f, 1.f };
		glm::vec4 endColor = { 1.f, 1.f, 1.f, 1.f };
		int totalLifetimeSeconds = 5;
		int spawnCount = 5;
		float scale = 0.0025f;
		int particleShape = 0; // 0 - square, 1 - circle // TODO enum or sth (fast impl for imgui exposure)...
	} properties;

	const std::size_t particlesLimit;

	struct InstanceData
	{
		glm::vec4 color;
		glm::mat4 transformation;
	};

	std::vector<InstanceData> instancesData;

public:
	InstancedParticleSystem(unsigned int pool)
		: VAO(genVertexArray())
		, VBO(genBuffer())
		, EBO(genBuffer())
		, instanceVBO(genBuffer())
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
		for (auto particle = aliveParticles.rbegin(); particle != aliveParticles.rend(); particle++)
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

		shader.use();
		shader.setMat4("view", view);
		shader.setMat4("projection", projection);

		const auto dataSize = sizeof(InstanceData) * instanceIndex;
		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, instancesData.data());

		glBindVertexArray(VAO);
		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instanceIndex);
	}

	auto& startColor() { return properties.startColor; }
	auto& endColor() { return properties.endColor; }
	auto& totalLifetimeSeconds() { return properties.totalLifetimeSeconds; }
	auto& spawnCount() { return properties.spawnCount; }
	auto& scale() { return properties.scale; }
	auto aliveParticlesCount() { return aliveParticles.size(); }
	auto& particleShape() { return properties.particleShape; }

	void emit(glm::vec3 worldPos, float t)
	{
		auto& [startColor, endColor, totalLifetimeSeconds, spawnCount, scale, shape] = properties;

		for (auto i = 0; i < spawnCount; i++)
		{
			if (aliveParticles.size() >= particlesLimit)
			{
				std::cerr << "All particles are alive." << std::endl;
				return; // cannot emit
			}

			auto particle = Particle{};
			particle.position = worldPos;
			particle.velocity = { rng::Float(), rng::Float(), rng::Float() };
			particle.acceleration = { 0.f, 0.f, 0.f };
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
};
