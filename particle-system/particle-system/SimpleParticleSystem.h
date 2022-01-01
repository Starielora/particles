#pragma once

#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glad/glad.h>

#include <iostream>
#include <string>
#include <cassert>
#include <vector>
#include <list>
#include <random>
#include <algorithm>

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

namespace
{
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

    constexpr float VERTICES[] = {
         0.5f,  0.5f, 0.0f,  // top right
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left 
    };

    constexpr unsigned int INDICES[] = {
        0, 1, 2,  // first Triangle
        2, 3, 0   // second Triangle
    };

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
}

class SimpleParticleSystem final
{
    const GLuint VAO, VBO, EBO;
    const Shader shader;

    struct Particle
    {
        glm::vec4 startColor;
        glm::vec4 endColor;
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 acceleration;
        float creationTime; // TODO chrono time_point? It has 2x size of float though...
        float rotationSpeed;
        float scale;
        bool isAlive = false;
    };

    std::list<Particle> aliveParticles;

    std::size_t size = sizeof(Particle);

    struct
    {
        glm::vec4 startColor = { 0.f, 1.f, 0.f, 1.f };
        glm::vec4 endColor = { 1.f, 1.f, 1.f, 1.f };
        int totalLifetimeSeconds = 5;
        int spawnCount = 5;
        float scale = 0.0025f;

    } properties;

    const std::size_t particlesLimit;

public:

    SimpleParticleSystem(unsigned int poolCount)
        : VAO(genVertexArray())
        , VBO(genBuffer())
        , EBO(genBuffer())
        , shader("D:/dev/particle-system/assets/simpleParticleSystem.vert", "D:/dev/particle-system/assets/simpleParticleSystem.frag") // TODO fix paths
        , particlesLimit(poolCount)
    {
        assert(VAO != 0);
        assert(VBO != 0);
        assert(EBO != 0);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    auto& startColor() { return properties.startColor; }
    auto& endColor() { return properties.endColor; }
    auto& totalLifetimeSeconds() { return properties.totalLifetimeSeconds; }
    auto& spawnCount() { return properties.spawnCount; }
    auto& scale() { return properties.scale; }
    auto aliveParticlesCount()
    {
        return aliveParticles.size();
    }

    void emit(glm::vec3 worldPos, float t)
    {
        auto& [startColor, endColor, totalLifetimeSeconds, spawnCount, scale] = properties;

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
            particle.rotationSpeed = rng::Float() * 10000;
            particle.startColor = startColor;
            particle.endColor = endColor;
            particle.scale = scale;
            particle.isAlive = true;
            aliveParticles.push_back(particle);
        }
    }

    void draw(glm::mat4 view, glm::mat4 projection, float currentTime)
    {
        const auto totalLifetimeSeconds = properties.totalLifetimeSeconds;

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
                auto model = glm::mat4(1.0f);
                model = glm::translate(model, particle.position);
                model = glm::rotate(model, particle.rotationSpeed * progress, glm::vec3(0, 0, 1));
                model = glm::scale(model, glm::vec3{ particle.scale, particle.scale, particle.scale });

                shader.use();
                shader.setMat4("model", model);
                shader.setMat4("view", view);
                shader.setMat4("projection", projection);
                shader.setVec4("color", color);

                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                particle.position += particle.velocity;
                particle.velocity += particle.acceleration;
            }
        }

        aliveParticles.remove_if([](const auto& p) { return !p.isAlive; });
    }
};
