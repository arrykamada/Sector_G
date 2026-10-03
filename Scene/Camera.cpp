#include "Scene/Camera.h"
#include "Core/Input.h"

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <cmath>

namespace SectorG {

    Camera::Camera()
        : m_Position(0.0f, 0.0f, 3.0f),
          m_Front(0.0f, 0.0f, -1.0f),
          m_Up(0.0f, 1.0f, 0.0f),
          m_Right(1.0f, 0.0f, 0.0f),
          m_WorldUp(0.0f, 1.0f, 0.0f),
          m_Yaw(-90.0f),    // смотрим в -Z
          m_Pitch(0.0f),
          m_Speed(3.0f),
          m_Sensitivity(0.15f)
    {
        UpdateVectors();
    }

    void Camera::Update(float dt) {
        // --- Мышь ---
        float dx = Input::GetMouseDeltaX();
        float dy = Input::GetMouseDeltaY();

        m_Yaw   += dx * m_Sensitivity;
        m_Pitch -= dy * m_Sensitivity;

        // Ограничиваем pitch, чтобы не перевернуться
        if (m_Pitch >  89.0f) m_Pitch =  89.0f;
        if (m_Pitch < -89.0f) m_Pitch = -89.0f;

        UpdateVectors();
        Input::ResetMouseDelta();

        // --- Клавиатура ---
        float velocity = m_Speed * dt;

        if (Input::IsKeyPressed(GLFW_KEY_W))
            m_Position += m_Front * velocity;
        if (Input::IsKeyPressed(GLFW_KEY_S))
            m_Position -= m_Front * velocity;
        if (Input::IsKeyPressed(GLFW_KEY_A))
            m_Position -= m_Right * velocity;
        if (Input::IsKeyPressed(GLFW_KEY_D))
            m_Position += m_Right * velocity;
        if (Input::IsKeyPressed(GLFW_KEY_SPACE))
            m_Position += m_WorldUp * velocity;
        if (Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
            m_Position -= m_WorldUp * velocity;
    }

    void Camera::UpdateVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        front.y = sin(glm::radians(m_Pitch));
        front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        m_Front = glm::normalize(front);

        m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
        m_Up    = glm::normalize(glm::cross(m_Right, m_Front));
    }

    glm::mat4 Camera::GetViewMatrix() const {
        return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
    }

    glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
        return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    }

}