#pragma once
#include <glm/glm.hpp>

namespace SectorG {

    class Camera {
    public:
        Camera();

        void Update(float dt);          // читает Input, двигает камеру
        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetProjectionMatrix(float aspect) const;

        glm::vec3 GetPosition() const { return m_Position; }

    private:
        glm::vec3 m_Position;
        glm::vec3 m_Front;
        glm::vec3 m_Up;
        glm::vec3 m_Right;
        glm::vec3 m_WorldUp;

        float m_Yaw;
        float m_Pitch;

        float m_Speed;
        float m_Sensitivity;

        void UpdateVectors();
    };

}