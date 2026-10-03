#pragma once

namespace SectorG {

    class Input {
    public:
        // Клавиши (WASD, Space, Shift)
        static bool IsKeyPressed(int keycode);

        // Мышь
        static float GetMouseDeltaX();
        static float GetMouseDeltaY();
        static void  ResetMouseDelta();

        // Курсор захвачен или нет
        static bool IsCursorCaptured();
        static void SetCursorCaptured(bool captured);

        // --- Callbacks (вызываются из Window) ---
        static void OnKey(int key, int action);
        static void OnMouseMove(double xpos, double ypos);

    private:
        static bool  s_Keys[1024];
        static float s_MouseDX;
        static float s_MouseDY;
        static float s_LastMouseX;
        static float s_LastMouseY;
        static bool  s_FirstMouse;
        static bool  s_CursorCaptured;
    };

}