#pragma once
#include <d3d9.h>

namespace UI {
    class Menu {
    public:
        static void Init(IDirect3DDevice9* pDevice, HWND hWnd);
        static void Render();
        static void Shutdown();

        static void Toggle();
        static bool IsVisible();

    private:
        static void ApplyTheme();
        static bool s_Initialized;
        static HWND s_Hwnd;
    };
}
