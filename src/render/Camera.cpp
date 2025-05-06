#include "render/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

Camera::Camera()
    : yaw_(0.0f), pitch_(0.0f), radius_(3.0f), 
    target_(0.0f, 1.0f, 0.0f), 
    rotating_(false), dragging_(false),
    lastX_(0.0), lastY_(0.0)
{}

glm::mat4 Camera::getViewMatrix() const
{
    float x = radius_ * cos(pitch_) * sin(yaw_);
    float y = radius_ * sin(pitch_);
    float z = radius_ * cos(pitch_) * cos(yaw_);
    glm::vec3 camPos = glm::vec3(x, y, z) + target_;

    // View matrix
    glm::mat4 view = glm::lookAt(
        camPos,          // camera in world
        target_,       // looking at origin (or any point)
        glm::vec3(0, 1, 0) // world’s up-vector
    );

    // Rotate the view so the model is upright
    view = view * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    return view;
}

void Camera::processMouseButton(int button, int action, double x, double y)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            rotating_ = true;
            lastX_ = x;
            lastY_ = y;
        }
        else if (action == GLFW_RELEASE) {
            rotating_ = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            dragging_ = true;
            lastX_ = x;
            lastY_ = y;
        }
        else if (action == GLFW_RELEASE) {
            dragging_ = false;
        }
    }
}

void Camera::processCursorPos(double xpos, double ypos)
{
    float dx = float(xpos - lastX_);
    float dy = float(ypos - lastY_);

    // rotating_
    if (rotating_) {
        yaw_ += dx * yawSpeed;
        pitch_ += dy * pitchSpeed;
        pitch_ = glm::clamp(pitch_, -glm::radians(89.0f), glm::radians(89.0f));
    }
    // dragging_
    else if (dragging_) {
        float x = radius_ * cos(pitch_) * sin(yaw_);
        float y = radius_ * sin(pitch_);
        float z = radius_ * cos(pitch_) * cos(yaw_);

        glm::vec3 camPos = glm::vec3(x, y, z) + target_;
        glm::vec3 forward = glm::normalize(target_ - camPos);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
        glm::vec3 up = glm::normalize(glm::cross(right, forward));

        target_ -= right * dx * dragSpeed;
        target_ += up * dy * dragSpeed;
    }
    lastX_ = xpos;
    lastY_ = ypos;
}

void Camera::processScroll(double yoffset)
{
    radius_ -= float(yoffset) * zoomSpeed;
    // Prevent going too close or too far
    if (radius_ < 1.0f)   radius_ = 1.0f;
    if (radius_ > 100.0f) radius_ = 100.0f;
}


