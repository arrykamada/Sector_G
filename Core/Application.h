#pragma once
#include <memory>

#include "Core/Window.h"
#include "Render/Renderer.h"
#include "Scene/Camera.h"

namespace SectorG {

    class Application {
    public:
        Application();
        ~Application();

        Application(const Application&)            = delete;
        Application& operator=(const Application&) = delete;

        void Run();

    private:
        std::unique_ptr<Window>   m_Window;
        std::unique_ptr<Renderer> m_Renderer;
        std::unique_ptr<Camera>   m_Camera;
        bool m_Running = true;
    };

}