#include "Core/Application.h"
#include "Utils/Log.h"

namespace SectorG {

    Application::Application() {
        Log::Info("Application created");

        m_Window = std::make_unique<Window>(WindowProps{
            "Sector_G", 1280, 720
        });

        m_Renderer = std::make_unique<Renderer>();
        m_Renderer->Init();

        m_Camera = std::make_unique<Camera>();
    }

    Application::~Application() {
        Log::Info("Application destroyed");
    }

    void Application::Run() {
        Log::Info("Main loop started");

        float time = 0.0f;
        float dt   = 0.016f;

        while (m_Running && !m_Window->ShouldClose()) {
            time += dt;
            m_Camera->Update(dt);

            float aspect = (float)m_Window->GetWidth() / (float)m_Window->GetHeight();
            glm::mat4 view       = m_Camera->GetViewMatrix();
            glm::mat4 projection = m_Camera->GetProjectionMatrix(aspect);

            m_Renderer->BeginFrame();
            m_Renderer->Draw(view, projection, m_Camera->GetPosition(), time);

            m_Window->OnUpdate();
        }

        Log::Info("Main loop stopped");
    }

}