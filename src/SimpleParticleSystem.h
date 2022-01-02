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

class SimpleParticleSystem final
{
    const GLuint VAO, VBO, EBO, textureId, FBO;
    const Shader shader;

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

    std::size_t size = sizeof(Particle);

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

public:

    SimpleParticleSystem(unsigned int poolCount, unsigned width, unsigned height)
        : VAO(gl::genVertexArray())
        , VBO(gl::genBuffer())
        , EBO(gl::genBuffer())
        , textureId(gl::genTexture(width, height))
        , FBO(gl::genFramebuffer(textureId))
        , shader("simpleParticleSystem.glsl")
        , particlesLimit(poolCount)
    {
        assert(VAO != 0);
        assert(VBO != 0);
        assert(EBO != 0);

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

    void draw(glm::mat4 view, glm::mat4 projection, float currentTime)
    {
        const auto totalLifetimeSeconds = properties.totalLifetimeSeconds;

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        gl::checkError();

        glClearColor(0.f, 0.f, 0.f, 0.f);
        gl::checkError();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::checkError();

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
                gl::checkError();
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                gl::checkError();

                particle.position += particle.velocity;
                particle.velocity += particle.acceleration;
            }
        }

        aliveParticles.remove_if([](const auto& p) { return !p.isAlive; });

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl::checkError();
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
