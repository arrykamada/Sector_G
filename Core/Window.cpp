#include "Core/Window.h"
#include "Core/Input.h"
#include "Utils/Log.h"

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace SectorG {

    static bool s_GLFWInitialized = false;

    Window::Window(const WindowProps& props) {
        Init(props);
    }

    Window::~Window() {
        Shutdown();
    }

    void Window::Init(const WindowProps& props) {
        m_Data.Title  = props.Title;
        m_Data.Width  = props.Width;
        m_Data.Height = props.Height;

        if (!s_GLFWInitialized) {
            if (!glfwInit()) {
                Log::Error("Failed to initialize GLFW");
                return;
            }
            s_GLFWInitialized = true;
            Log::Info("GLFW initialized");
        }

        // OpenGL 3.3 Core context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_Window = glfwCreateWindow(
            (int)m_Data.Width,
            (int)m_Data.Height,
            m_Data.Title.c_str(),
            nullptr, nullptr
        );

        if (!m_Window) {
            Log::Error("Failed to create GLFW window");
            glfwTerminate();
            s_GLFWInitialized = false;
            return;
        }

        glfwMakeContextCurrent(m_Window);
        glfwSwapInterval(1); // VSync

        // GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            Log::Error("Failed to initialize GLEW");
            return;
        }

        Log::Info("OpenGL loaded: " +
                  std::string((char*)glGetString(GL_VERSION)));

        // --- Input callbacks ---
        glfwSetKeyCallback(m_Window, [](GLFWwindow*, int key, int, int action, int) {
            Input::OnKey(key, action);
        });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow*, double x, double y) {
            Input::OnMouseMove(x, y);
        });

        // Захват курсора
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        Input::SetCursorCaptured(true);

        Log::Info("Window created: " + m_Data.Title + " (" +
                  std::to_string(m_Data.Width) + "x" +
                  std::to_string(m_Data.Height) + ")");
    }

    void Window::Shutdown() {
        if (m_Window) {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }
        if (s_GLFWInitialized) {
            glfwTerminate();
            s_GLFWInitialized = false;
            Log::Info("GLFW terminated");
        }
    }

    void Window::OnUpdate() {
        glfwPollEvents();
        glfwSwapBuffers(m_Window);
    }

    bool Window::ShouldClose() const {
        return glfwWindowShouldClose(m_Window);
    }

}