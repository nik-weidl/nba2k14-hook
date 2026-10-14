#pragma once
#include <windows.h>

namespace Hooks {
    class WndProcHook {
    public:
        static void Init(HWND hWnd);
        static void Shutdown();

    private:
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static WNDPROC s_OriginalWndProc;
        static HWND s_Hwnd;
    };
}
