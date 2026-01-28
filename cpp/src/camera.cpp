#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

Camera::Camera(glm::vec3 pos) : position(pos) {}

glm::vec3 Camera::getFront() const {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    return glm::normalize(front);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + getFront(), glm::vec3(0, 1, 0));
}

void Camera::processKeyboard(int forward, int right, int up, float dt) {
    glm::vec3 front = getFront();
    glm::vec3 flatFront = glm::normalize(glm::vec3(front.x, 0, front.z));
    glm::vec3 rightDir = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));

    float vel = speed * dt;
    position += flatFront * (float)forward * vel;
    position += rightDir * (float)right * vel;
    position.y += (float)up * vel;
}

void Camera::processMouse(float dx, float dy) {
    yaw += dx * sensitivity;
    pitch += dy * sensitivity;
    pitch = std::clamp(pitch, -89.0f, 89.0f);
}
