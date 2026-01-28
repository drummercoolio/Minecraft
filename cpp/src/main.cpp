#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>

#include "shader.h"
#include "camera.h"
#include "world.h"
#include "texture.h"

static Camera camera(glm::vec3(0, 80, 0));
static bool firstMouse = true;
static double lastX = 400, lastY = 300;
static bool wireframe = false;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    camera.processMouse((float)(xpos - lastX), (float)(lastY - ypos));
    lastX = xpos;
    lastY = ypos;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    }
}

int main(int argc, char** argv) {
    int seed = 42;
    if (argc > 1) seed = atoi(argv[1]);

    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Minecraft Clone", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW" << std::endl;
        return 1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // sky blue

    Shader shader;
    if (!shader.load("shaders/vertex.glsl", "shaders/fragment.glsl")) {
        std::cerr << "Failed to load shaders" << std::endl;
        return 1;
    }

    GLuint atlas = generateTextureAtlas();

    World world(seed);
    std::cout << "Generating world with seed " << seed << "..." << std::endl;

    // Place camera at a good height
    world.update(camera.position);
    // Wait for initial chunks
    for (int i = 0; i < 20; i++)
        world.update(camera.position);

    int spawnHeight = world.getHeight(0, 0);
    camera.position = glm::vec3(0.5f, (float)(spawnHeight + 2), 0.5f);
    std::cout << "Spawn at y=" << spawnHeight + 2 << std::endl;

    float lastTime = (float)glfwGetTime();
    int frameCount = 0;
    float fpsTimer = 0;

    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        // FPS counter
        frameCount++;
        fpsTimer += dt;
        if (fpsTimer >= 1.0f) {
            char title[64];
            snprintf(title, sizeof(title), "Minecraft Clone | FPS: %d | Pos: %.0f, %.0f, %.0f",
                     frameCount, camera.position.x, camera.position.y, camera.position.z);
            glfwSetWindowTitle(window, title);
            frameCount = 0;
            fpsTimer = 0;
        }

        // Input
        glfwPollEvents();
        int fwd = 0, right = 0, up = 0;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) fwd++;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) fwd--;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) right++;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) right--;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) up++;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) up--;

        // Sprint
        float speedMult = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            speedMult = 3.0f;
        camera.speed = 10.0f * speedMult;

        camera.processKeyboard(fwd, right, up, dt);

        // World update
        world.update(camera.position);

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (height == 0) height = 1;
        glViewport(0, 0, width, height);

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.fov), (float)width / height, 0.1f, 300.0f);
        glm::mat4 view = camera.getViewMatrix();

        shader.use();
        shader.setMat4("uProjection", projection);
        shader.setMat4("uView", view);
        shader.setInt("uTexture", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas);

        world.render();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
