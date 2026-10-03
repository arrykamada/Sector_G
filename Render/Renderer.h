#pragma once
#include <glm/glm.hpp>

namespace SectorG {

    class Renderer {
    public:
        Renderer();
        ~Renderer();

        Renderer(const Renderer&)            = delete;
        Renderer& operator=(const Renderer&) = delete;

        void Init();
        void BeginFrame();

        void Draw(const glm::mat4& view,
                  const glm::mat4& projection,
                  const glm::vec3& cameraPos,
                  float time);

    private:
        unsigned int m_ShaderProgram = 0;
        unsigned int m_ShadowProgram = 0;

        unsigned int m_CubeVAO = 0;
        unsigned int m_CubeVBO = 0;
        unsigned int m_CubeEBO = 0;

        unsigned int m_FloorVAO = 0;
        unsigned int m_FloorVBO = 0;

        unsigned int m_CubeTexture  = 0;
        unsigned int m_FloorTexture = 0;

        int m_ModelLoc = -1;
        int m_ViewLoc  = -1;
        int m_ProjLoc  = -1;

        int m_SModelLoc  = -1;
        int m_SViewLoc   = -1;
        int m_SProjLoc   = -1;
        int m_SLightLoc  = -1;
        int m_SGroundLoc = -1;
    };

}