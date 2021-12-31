#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
//#include "SimpleParticleSystem.h"
//#include "BatchParticleSystem.h"
#include "InstancedParticleSystem.h"
#include "Containers.h"
#include "Timer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
//void processInput(GLFWwindow* window, SimpleParticleSystem& particleSystem, float t);
//void processInput(GLFWwindow* window, BatchParticleSystem& particleSystem, float t);
void processInput(GLFWwindow* window, InstancedParticleSystem& particleSystem, float t);

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
auto CURRENT_WIDTH = SCR_WIDTH;
auto CURRENT_HEIGHT = SCR_HEIGHT;

// camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float cameraSpeedMultiplier = 2.5f;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

std::vector<float> fpsValues(100);
std::vector<float> particlesDrawTimes;

// particle system
auto startColor = glm::vec4(0.f, 1.f, 0.f, 1.f);
auto endColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
auto particleScale = float{ 0.0025 };
auto spawnCount = 5;
auto particleLifeTimeSeconds = int{ 5 };

auto getProjectionMatrix()
{
    return glm::perspective(glm::radians(fov), (float)CURRENT_WIDTH / (float)CURRENT_HEIGHT, 0.1f, 100.f);
}

auto getViewMatrix()
{
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void insertFpsVal(float v)
{
    fpsValues.erase(fpsValues.begin()); // TODO rather heavy
    fpsValues.push_back(v);
}

namespace postprocessing
{
    GLuint sceneFBO, colorBuffers[2], RBO;
    Shader shader;
    GLuint VAO, VBO, EBO;

    namespace blur
    {
        GLuint horizontalFBO, verticalFBO;
        GLuint horizontalColorBuffer, verticalColorBuffer;
        Shader shader;
    }

    void generateFramebuffers()
    {
        {
            glGenFramebuffers(1, &sceneFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

            // two render targets for scene FBO
            glGenTextures(2, colorBuffers);
            auto& [colorBuffer1, colorBuffer2] = colorBuffers;

            // 1
            glBindTexture(GL_TEXTURE_2D, colorBuffer1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CURRENT_WIDTH, CURRENT_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer1, 0);

            // 2 
            glBindTexture(GL_TEXTURE_2D, colorBuffer2);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CURRENT_WIDTH, CURRENT_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, colorBuffer2, 0);

            // RBO
            glGenRenderbuffers(1, &RBO);
            glBindRenderbuffer(GL_RENDERBUFFER, RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, CURRENT_WIDTH, CURRENT_HEIGHT);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

            // draw to two buffers
            unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, attachments);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("Framebuffer not complete.");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        {
            using namespace blur;
            glGenFramebuffers(1, &horizontalFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, horizontalFBO);

            // TODO add texture resize to window resizing
            glGenTextures(1, &horizontalColorBuffer);
            glBindTexture(GL_TEXTURE_2D, horizontalColorBuffer);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CURRENT_WIDTH, CURRENT_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, horizontalColorBuffer, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("Framebuffer not complete.");

            glGenFramebuffers(1, &verticalFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, verticalFBO);

            glGenTextures(1, &verticalColorBuffer);
            glBindTexture(GL_TEXTURE_2D, verticalColorBuffer);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CURRENT_WIDTH, CURRENT_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, verticalColorBuffer, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("Framebuffer not complete.");

            glBindTexture(GL_TEXTURE_2D, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void resize()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

        auto& [colorBuffer1, colorBuffer2] = colorBuffers;
        glBindTexture(GL_TEXTURE_2D, colorBuffer1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CURRENT_WIDTH, CURRENT_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer1, 0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CURRENT_WIDTH, CURRENT_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer2, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, CURRENT_WIDTH, CURRENT_HEIGHT);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void loadShader()
    {
        shader = Shader("D:/dev/particle-system/assets/postprocessingShader.vert", "D:/dev/particle-system/assets/postprocessingShader.frag");
        blur::shader = Shader("D:/dev/particle-system/assets/blurShader.vert", "D:/dev/particle-system/assets/blurShader.frag");

        shader.use();
        shader.setInt("scene", 0);
        shader.setInt("bloomBlur", 1);
    }

    void createVAO()
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        constexpr float VERTICES[] = {
            -1.f, -1.f, 0.f, 0.f, // bl
             1.f, -1.f, 1.f, 0.f, // br
             1.f,  1.f, 1.f, 1.f, // tr
            -1.f,  1.f, 0.f, 1.f  // tl
        };

        constexpr unsigned int INDICES[] = { 0, 1, 2, 2, 3, 0 };

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    void draw()
    {
        postprocessing::blur::shader.use();
        glBindFramebuffer(GL_FRAMEBUFFER, blur::horizontalFBO);
        postprocessing::blur::shader.setInt("horizontal", true); // TODO setBool?
        glBindTexture(GL_TEXTURE_2D, postprocessing::colorBuffers[1]);
        glBindVertexArray(postprocessing::VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        for (auto i = 0u; i < 10; ++i)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, blur::verticalFBO);
            postprocessing::blur::shader.setInt("horizontal", false);
            glBindTexture(GL_TEXTURE_2D, blur::horizontalColorBuffer);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, blur::horizontalFBO);
            postprocessing::blur::shader.setInt("horizontal", true);
            glBindTexture(GL_TEXTURE_2D, blur::verticalColorBuffer);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glDisable(GL_DEPTH_TEST);
        shader.use();
        shader.setInt("bloom", true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, postprocessing::colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, blur::horizontalColorBuffer);
        glActiveTexture(GL_TEXTURE0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
}

int main()
{
    particlesDrawTimes.resize(1000);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "particle-system", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    postprocessing::generateFramebuffers();
    postprocessing::loadShader();
    postprocessing::createVAO();

    auto containers = Containers();
    // TODO has to be after opengl init because constructor uses opengl
    auto particleSystem = InstancedParticleSystem(500e3);
    //auto particleSystem = BatchParticleSystem(500e3);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 1.25f;

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    bool show_demo_window = false;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        insertFpsVal(1 / deltaTime);

        processInput(window, particleSystem, currentFrame);

        const auto view = getViewMatrix();
        const auto projection = getProjectionMatrix();

        glBindFramebuffer(GL_FRAMEBUFFER, postprocessing::sceneFBO);
        glEnable(GL_DEPTH_TEST);

        //glClearColor(0.1f, 0.16f, 0.39f, 1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //containers.draw(view, projection);

        {
            const auto timer = Timer<std::chrono::milliseconds>(particlesDrawTimes);
            particleSystem.draw(view, projection, currentFrame);
        }

        postprocessing::draw();
        
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            // camera ui
            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Camera");

                ImGui::SliderFloat("Movement speed", &cameraSpeedMultiplier, 0.0f, 100.0f);
                ImGui::SliderFloat3("Position", &cameraPos[0], -50.f, 50.f);
                ImGui::SliderFloat("FOV", &fov, 1.0f, 90.0f);

                ImGui::End();
            }

            // fps plot
            {
                ImGui::Begin("FPS");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::PlotLines("", &fpsValues[0], fpsValues.size(), 0, nullptr, 1.f, 144.0f, ImVec2(0, 100.0f));
                ImGui::End();
            }

            // particle system
            {
                ImGui::Begin("Particle system");
                ImGui::Text((std::string("Alive particles: ") + std::to_string(particleSystem.aliveParticlesCount())).c_str());
                ImGui::ColorEdit4("Start", &particleSystem.startColor()[0]);
                ImGui::ColorEdit4("End", &particleSystem.endColor()[0]);
                ImGui::SliderFloat("Scale", &particleSystem.scale(), 0.f, 1.f);
                ImGui::SliderInt("Spawn count", &particleSystem.spawnCount(), 0, 1000);
                ImGui::SliderInt("Life time [s]", &particleSystem.totalLifetimeSeconds(), 0, 100);
                ImGui::PlotLines("draw [ms]", &particlesDrawTimes[0], particlesDrawTimes.size(), 0, nullptr, 0.f, 16.f, ImVec2(0, 100.f));
                ImGui::RadioButton("Square", &particleSystem.particleShape(), 0); ImGui::SameLine(); ImGui::RadioButton("Circle", &particleSystem.particleShape(), 1);
                //ImGui::PlotLines("vertices sub data [bytes]", &subDataBytes[0], subDataBytes.size(), 0, nullptr, 0.f, 1e7, ImVec2(0, 100.f));
                //ImGui::PlotLines("addQuads [ms]", &addQuadsTimes[0], addQuadsTimes.size(), 0, nullptr, 0.f, 50, ImVec2(0, 100.f));
                //ImGui::PlotLines("iteration [ms]", &iterationTimes[0], iterationTimes.size(), 0, nullptr, 0.f, 50, ImVec2(0, 100.f));
                //ImGui::PlotLines("gl [ms]", &glStuffTimes[0], glStuffTimes.size(), 0, nullptr, 0.f, 50, ImVec2(0, 100.f));
                ImGui::End();
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

//void processInput(GLFWwindow* window, BatchParticleSystem& particleSystem, float t)
void processInput(GLFWwindow* window, InstancedParticleSystem& particleSystem, float t)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = cameraSpeedMultiplier * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        const auto xTrans = ((xpos / CURRENT_WIDTH) * 2 - 1);
        const auto yTrans = ((ypos / CURRENT_HEIGHT) * 2 - 1);

        const auto invProjection = glm::inverse(getProjectionMatrix()); // go back to camera coordinates
        const auto offsetFromCamera = glm::vec3(invProjection * glm::vec4{ xTrans,-yTrans, 1, 1 });

        auto worldPos = cameraPos + offsetFromCamera;
        {
            if (ImGui::GetIO().Framerate > 59.f)
                particleSystem.emit(worldPos, t);
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    CURRENT_WIDTH = width;
    CURRENT_HEIGHT = height;
    postprocessing::resize();
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
    {
        //if (firstMouse)
        //{
        //    lastX = xpos;
        //    lastY = ypos;
        //    firstMouse = false;
        //}

        //float xoffset = xpos - lastX;
        //float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        //float sensitivity = 0.1f; // change this value to your liking
        //xoffset *= sensitivity;
        //yoffset *= sensitivity;

        //yaw += xoffset;
        //pitch += yoffset;

        //// make sure that when pitch is out of bounds, screen doesn't get flipped
        //if (pitch > 89.0f)
        //    pitch = 89.0f;
        //if (pitch < -89.0f)
        //    pitch = -89.0f;

        //glm::vec3 front;
        //front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        //front.y = sin(glm::radians(pitch));
        //front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        //cameraFront = glm::normalize(front);
    }

    lastX = xpos;
    lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

