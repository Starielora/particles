#pragma once

#include "shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace
{
    constexpr auto N = 100u;

    void generate_grid(std::vector<glm::vec3>& vertices, std::vector<glm::uvec3>& indices)
    {
        // TODO fix whatever is happening here - copied code :(
        for (int j = 0; j <= N; ++j)
        {
            for (int i = 0; i <= N; ++i)
            {
                float x = (float)i;
                float y = (float)j;
                float z = 0;//(float)j / (float)N;//f(x, y);
                vertices.push_back(glm::vec3(x, y, z));
            }
        }

        for (int j = 0; j < N; ++j)
        {
            for (int i = 0; i < N; ++i)
            {
                int row1 = j * (N + 1);
                int row2 = (j + 1) * (N + 1);

                // triangle 1
                indices.push_back(glm::uvec3(row1 + i, row1 + i + 1, row2 + i + 1));

                // triangle 2
                indices.push_back(glm::uvec3(row1 + i, row2 + i + 1, row2 + i));
            }
        }
    }

    GLuint generate_vao()
    {
        std::vector<glm::vec3> vertices;
        std::vector<glm::uvec3> indices;

        generate_grid(vertices, indices);

        GLuint  vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), glm::value_ptr(vertices[0]), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

        GLuint ibo;
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(glm::uvec3), glm::value_ptr(indices[0]), GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return vao;
    }
}

class Grid final
{
    const unsigned int vao;
    const Shader shader;
public:
    Grid()
        : vao(generate_vao())
        , shader("D:/dev/particle-system/assets/grid.vert", "D:/dev/particle-system/assets/grid.frag")
    {

    }

    void draw(glm::mat4 view, glm::mat4 projection)
    {
        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        auto model = glm::mat4(1.0f);
        model = glm::translate(model, { -10.f, -10.f, 0.0f });
        shader.setMat4("model", model);
        glBindVertexArray(vao);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, (GLsizei)N * N, GL_UNSIGNED_INT, NULL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindVertexArray(0);
    }
};
