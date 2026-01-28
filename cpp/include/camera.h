#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 position;
    float yaw = -90.0f;
    float pitch = 0.0f;
    float speed = 10.0f;
    float sensitivity = 0.1f;
    float fov = 70.0f;

    Camera(glm::vec3 pos = glm::vec3(0, 80, 0));

    glm::mat4 getViewMatrix() const;
    glm::vec3 getFront() const;

    void processKeyboard(int forward, int right, int up, float dt);
    void processMouse(float dx, float dy);
};
