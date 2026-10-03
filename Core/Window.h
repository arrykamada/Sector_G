#pragma once
#include <string>

struct GLFWwindow; // forward declaration — не тянем GLFW в заголовок

namespace SectorG {

    struct WindowProps {
        std::string  Title;
        unsigned int Width;
        unsigned int Height;

        WindowProps(const std::string& title = "Sector_G",
                    unsigned int width  = 1280,
                    unsigned int height = 720)
            : Title(title), Width(width), Height(height) {}
    };

    class Window {
    public:
        Window(const WindowProps& props = WindowProps());
        ~Window();

        // Запрещаем копирование — окно должно быть одно
        Window(const Window&)            = delete;
        Window& operator=(const Window&) = delete;

        void OnUpdate();          // обработка событий + swap buffers
        bool ShouldClose() const;

        unsigned int GetWidth()  const { return m_Data.Width; }
        unsigned int GetHeight() const { return m_Data.Height; }
        const std::string& GetTitle() const { return m_Data.Title; }

        GLFWwindow* GetNativeWindow() const { return m_Window; }

    private:
        void Init(const WindowProps& props);
        void Shutdown();

        GLFWwindow* m_Window = nullptr;

        struct WindowData {
            std::string  Title;
            unsigned int Width;
            unsigned int Height;
        } m_Data;
    };

}