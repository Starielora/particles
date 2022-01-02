#pragma once

#include "shader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>

#include <iostream>
#include <chrono>
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

class Particle final
{
    static constexpr float VERTICES[] = {
         0.5f,  0.5f, 0.0f,  // top right
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left 
    };

    static constexpr unsigned int INDICES[] = {
        0, 1, 2,  // first Triangle
        2, 3, 0   // second Triangle
    };

    static unsigned int VBO, VAO, EBO;
    static Shader shader;
    glm::vec3 position;
    glm::vec3 velocity = { rng::Float(), rng::Float(), rng::Float() };
    glm::vec3 acceleration = { 0, 0, 0 };

    bool _isAlive = false;
    std::chrono::steady_clock::time_point _created;
    std::chrono::seconds _totalLifetime;
    float rotation = rng::Float() * 10000;

    void init()
    {
        if (VAO == 0 && EBO == 0 && VBO == 0)
        {
            shader = Shader("D:/dev/particle-system/assets/quadVertexShader.glsl", "D:/dev/particle-system/assets/quadFragmentShader.glsl");

            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }

public:
    Particle()
    {
        init();
    }

    Particle(glm::vec3 position, std::chrono::seconds lifetime)
        : position(std::move(position))
        , _created(std::chrono::steady_clock::now())
        , _totalLifetime(std::move(lifetime))
    {
        init();
    }

    bool isAlive() const { return _isAlive; }

    void emit(glm::vec3 pos, std::chrono::seconds lifetime)
    {
        position = pos;
        _created = std::chrono::steady_clock::now();
        _totalLifetime = lifetime;
        _isAlive = true;
    }

    void draw(glm::mat4 view, glm::mat4 projection, glm::vec4 rgbStart, glm::vec4 rgbEnd, float scale)
    {
        const auto now = std::chrono::steady_clock::now();
        const auto lifetime = std::chrono::duration_cast<std::chrono::milliseconds>(now - _created);
        if (_isAlive && lifetime < _totalLifetime)
        {
            const auto progress = float(lifetime.count()) / float(std::chrono::duration_cast<std::chrono::milliseconds>(_totalLifetime).count());

            // color
            const auto color = glm::lerp(rgbStart, rgbEnd, progress);

            shader.use();
            auto model = glm::mat4(1.0f);
            model = glm::translate(model, position);
            position += velocity;
            velocity += acceleration;
            model = glm::rotate(model, rotation * progress, glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3{ scale, scale, scale });
            shader.setMat4("model", model);
            shader.setMat4("view", view);
            shader.setMat4("projection", projection);
            shader.setVec4("color", color);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        else
        {
            _isAlive = false;
        }
    }
};

unsigned int Particle::EBO = 0;
unsigned int Particle::VAO = 0;
unsigned int Particle::VBO = 0;
Shader Particle::shader;
