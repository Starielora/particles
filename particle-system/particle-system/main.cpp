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
#include "GaussianBlur.h"
#include "AdditiveBlend.h"
#include "Camera.h"

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

bool firstMouse = true;
auto camera = Camera{};

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

std::vector<float> fpsValues(100);
std::vector<float> particlesDrawTimes;

InstancedParticleSystem* particleSystemPtr;
GaussianBlur* gaussianBlurPtr;
AdditiveBlend* additiveBlendPtr;

void insertFpsVal(float v)
{
    fpsValues.erase(fpsValues.begin());
    fpsValues.push_back(v);
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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto quadTextureShader = Shader("D:/dev/particle-system/assets/vertexShader.glsl", "D:/dev/particle-system/assets/fragmentShader.glsl");
    quadTextureShader.use();
    quadTextureShader.setInt("sceneTexture", 0);

    auto containers = Containers();
    // TODO has to be after opengl init because constructor uses opengl
    //auto particleSystem = BatchParticleSystem(500e3);
    const auto quad = TexturedQuad{};
    auto particleSystem = InstancedParticleSystem(500e3, CURRENT_WIDTH, CURRENT_HEIGHT);
    auto gaussianBlur = GaussianBlur(CURRENT_WIDTH, CURRENT_HEIGHT, quad);
    auto additiveBlend = AdditiveBlend(CURRENT_WIDTH, CURRENT_HEIGHT, quad);

    // TODO send help
    particleSystemPtr = &particleSystem;
    gaussianBlurPtr = &gaussianBlur;
    additiveBlendPtr = &additiveBlend;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 1.25f;

    // Setup Dear ImGui style
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    bool show_demo_window = false;
    bool blur = true, bloom = true;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        insertFpsVal(1 / deltaTime);

        processInput(window, particleSystem, currentFrame);

        const auto view = camera.view();
        const auto projection = camera.projection(CURRENT_WIDTH, CURRENT_HEIGHT);

        glEnable(GL_DEPTH_TEST);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto finalTexture = particleSystem.texture();
        {
            const auto timer = Timer<std::chrono::milliseconds>(particlesDrawTimes);
            particleSystem.draw(view, projection, currentFrame);
        }

        // blur / bloom
        if (blur)
        {
            gaussianBlur.draw(particleSystem.texture());
            finalTexture = gaussianBlur.texture();

            if (bloom)
            {
                additiveBlend.draw(particleSystem.texture(), gaussianBlur.texture());
                finalTexture = additiveBlend.texture();
            }
        }

        {
            quadTextureShader.use();
            glBindVertexArray(quad.VAO());
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, finalTexture);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        
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

                ImGui::SliderFloat("Movement speed", &camera.speedMultiplier(), 0.0f, 100.0f);
                ImGui::SliderFloat3("Position", &camera.position()[0], -50.f, 50.f);
                ImGui::SliderFloat("FOV", &camera.fov(), 1.0f, 90.0f);

                ImGui::End();
            }

            // fps plot
            {
                ImGui::Begin("FPS");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::PlotLines("", &fpsValues[0], fpsValues.size(), 0, nullptr, 1.f, 144.0f, ImVec2(0, 100.0f));
                ImGui::PlotLines("draw [ms]", &particlesDrawTimes[0], particlesDrawTimes.size(), 0, nullptr, 0.f, 16.f, ImVec2(0, 100.f));
                ImGui::End();
            }

            // particle system
            {
                ImGui::Begin("Particle system");
                ImGui::Text((std::string("Alive particles: ") + std::to_string(particleSystem.aliveParticlesCount())).c_str());
                ImGui::ColorEdit4("Start", &particleSystem.startColor()[0]);
                ImGui::ColorEdit4("End", &particleSystem.endColor()[0]);

                ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25);
                ImGui::Text("Initial velocity");
                ImGui::SameLine();
                ImGui::Checkbox("Random velocity", &particleSystem.randomVelocity());
                ImGui::BeginDisabled(particleSystem.randomVelocity());
                ImGui::DragFloat("vx", &particleSystem.initialVelocity()[0], 0.0001, -0.05f, 0.05f);
                ImGui::SameLine();
                ImGui::DragFloat("vy", &particleSystem.initialVelocity()[1], 0.0001, -0.05f, 0.05f);
                ImGui::SameLine();
                ImGui::DragFloat("vz", &particleSystem.initialVelocity()[2], 0.0001, -0.05f, 0.05f);
                ImGui::EndDisabled();

                ImGui::Text("Acceleration");
                ImGui::SameLine();
                ImGui::Checkbox("Random acceleration", &particleSystem.randomAcceleration());
                ImGui::BeginDisabled(particleSystem.randomAcceleration());
                ImGui::DragFloat("ax", &particleSystem.acceleration()[0], 0.00001, -0.0005f, 0.0005f, "%.4f");
                ImGui::SameLine();
                ImGui::DragFloat("ay", &particleSystem.acceleration()[1], 0.00001, -0.0005f, 0.0005f, "%.4f");
                ImGui::SameLine();
                ImGui::DragFloat("az", &particleSystem.acceleration()[2], 0.00001, -0.0005f, 0.0005f, "%.4f");
                ImGui::EndDisabled();
                ImGui::PopItemWidth();

                ImGui::SliderFloat("Scale", &particleSystem.scale(), 0.f, 0.05f);
                ImGui::SliderInt("Spawn count", &particleSystem.spawnCount(), 1, 1000);
                ImGui::SliderInt("Life time [s]", &particleSystem.totalLifetimeSeconds(), 0, 100);
                ImGui::RadioButton("Square", &particleSystem.particleShape(), 0); ImGui::SameLine(); ImGui::RadioButton("Circle", &particleSystem.particleShape(), 1); ImGui::SameLine(); ImGui::RadioButton("Triangle", &particleSystem.particleShape(), 2);
                ImGui::SliderFloat("Thickness", &particleSystem.shapeThickness(), 0.0f, 1.f);

                ImGui::Checkbox("Gaussian blur", &blur);
                ImGui::BeginDisabled(!blur);
                ImGui::SliderInt("Iterations", &gaussianBlur.iterations(), 1, 20);
                ImGui::Checkbox("Bloom", &bloom);
                ImGui::BeginDisabled(!bloom);
                ImGui::SliderFloat("Factor", &additiveBlend.factor(), 0.0f, 10.f);
                ImGui::EndDisabled();
                ImGui::EndDisabled();

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

    //float cameraSpeed = cameraSpeedMultiplier * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.moveForward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.moveBack(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.strafeLeft(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.strafeRight(deltaTime);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        const auto xTrans = ((xpos / CURRENT_WIDTH) * 2 - 1);
        const auto yTrans = ((ypos / CURRENT_HEIGHT) * 2 - 1);

        const auto invProjection = glm::inverse(camera.projection(CURRENT_WIDTH, CURRENT_HEIGHT)); // go back to camera coordinates
        const auto offsetFromCamera = glm::vec3(invProjection * glm::vec4{ xTrans,-yTrans, 1, 1 });

        auto worldPos = camera.position() + offsetFromCamera;
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

    particleSystemPtr->resize(width, height);
    gaussianBlurPtr->resize(width, height);
    additiveBlendPtr->resize(width, height);
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

    //lastX = xpos;
    //lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.fov(-yoffset);
}

