cmake_minimum_required(VERSION 3.10)
project(MinecraftClone)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(minecraft
    src/main.cpp
    src/shader.cpp
    src/camera.cpp
    src/chunk.cpp
    src/chunk_mesh.cpp
    src/world.cpp
    src/texture.cpp
    src/block.cpp
)

target_include_directories(minecraft PRIVATE include lib)
target_link_libraries(minecraft glfw GLEW::GLEW OpenGL::GL)

# Copy shaders to build dir
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})
