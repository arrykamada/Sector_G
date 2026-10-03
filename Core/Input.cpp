#include "Core/Input.h"

#include <GLFW/glfw3.h>

namespace SectorG {

    bool  Input::s_Keys[1024]     = { false };
    float Input::s_MouseDX        = 0.0f;
    float Input::s_MouseDY        = 0.0f;
    float Input::s_LastMouseX     = 0.0f;
    float Input::s_LastMouseY     = 0.0f;
    bool  Input::s_FirstMouse     = true;
    bool  Input::s_CursorCaptured = false;

    bool Input::IsKeyPressed(int keycode) {
        if (keycode < 0 || keycode >= 1024) return false;
        return s_Keys[keycode];
    }

    float Input::GetMouseDeltaX() { return s_MouseDX; }
    float Input::GetMouseDeltaY() { return s_MouseDY; }

    void Input::ResetMouseDelta() {
        s_MouseDX = 0.0f;
        s_MouseDY = 0.0f;
    }

    bool Input::IsCursorCaptured()          { return s_CursorCaptured; }
    void Input::SetCursorCaptured(bool cap) { s_CursorCaptured = cap; }

    void Input::OnKey(int key, int action) {
        if (key < 0 || key >= 1024) return;

        if (action == GLFW_PRESS)
            s_Keys[key] = true;
        else if (action == GLFW_RELEASE)
            s_Keys[key] = false;
    }

    void Input::OnMouseMove(double xpos, double ypos) {
        if (!s_CursorCaptured) return;

        if (s_FirstMouse) {
            s_LastMouseX = (float)xpos;
            s_LastMouseY = (float)ypos;
            s_FirstMouse = false;
        }

        s_MouseDX += (float)xpos - s_LastMouseX;
        s_MouseDY += (float)ypos - s_LastMouseY;

        s_LastMouseX = (float)xpos;
        s_LastMouseY = (float)ypos;
    }

}