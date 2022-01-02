cmake_minimum_required(VERSION 3.22)

project(particles)

    add_subdirectory(deps)

    add_executable(particles 
        src/main.cpp

        src/AdditiveBlend.h
        src/BatchParticleSystem.h
        src/Camera.h
        src/Containers.h
        src/GaussianBlur.h
        src/Grid.h
        src/InstancedParticleSystem.h
        src/OpenGLUtils.h
        src/Particle.h
        src/Shader.h
        src/SimpleParticleSystem.h
        src/TexturedQuad.h
        src/Timer.h
    )

    target_link_libraries(particles glad glm glfw Dear-ImGui)

    set_target_properties(particles PROPERTIES CXX_STANDARD 17)
    # TODO add
    # /DEBUG - to include debug information in Release build
    # /Zi - debug info db
    # /W3 - more warnings
    # /sdl - Security Development Cycle
    # /GL - optimize whole app
    # /LTCG:incremental
    # some more?
