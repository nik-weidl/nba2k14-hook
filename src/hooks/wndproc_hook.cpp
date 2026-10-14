#include "wndproc_hook.h"
#include "../ui/menu.h"
#include "../core/logger.h"
#include <imgui.h>

// Forward declare ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Hooks {
    WNDPROC WndProcHook::s_OriginalWndProc = nullptr;
    HWND WndProcHook::s_Hwnd = nullptr;

    void WndProcHook::Init(HWND hWnd) {
        if (s_OriginalWndProc) return;
        s_Hwnd = hWnd;
        s_OriginalWndProc = (WNDPROC)SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
        LOG_INFO("WndProc hooked (Original: 0x%p)", s_OriginalWndProc);
    }

    void WndProcHook::Shutdown() {
        if (s_OriginalWndProc && s_Hwnd) {
            SetWindowLongPtrA(s_Hwnd, GWLP_WNDPROC, (LONG_PTR)s_OriginalWndProc);
            s_OriginalWndProc = nullptr;
            s_Hwnd = nullptr;
            LOG_INFO("WndProc unhooked");
        }
    }

    LRESULT CALLBACK WndProcHook::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        // Toggle key detection (F8 = 0x77, Insert = 0x2D)
        if (uMsg == WM_KEYDOWN) {
            if (wParam == VK_F8 || wParam == VK_INSERT) {
                UI::Menu::Toggle();
                return 0; // Consume toggle key
            }
        }

        // Pass events to ImGui if menu is open
        if (UI::Menu::IsVisible()) {
            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
            
            ImGuiIO& io = ImGui::GetIO();
            // Block game mouse events if clicking on ImGui
            if (io.WantCaptureMouse && (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP ||
                                        uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP ||
                                        uMsg == WM_MOUSEMOVE || uMsg == WM_MOUSEWHEEL)) {
                return 0;
            }

            // Block game keyboard events if typing in ImGui text input
            if (io.WantCaptureKeyboard && (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR)) {
                return 0;
            }
        }

        return CallWindowProcA(s_OriginalWndProc, hWnd, uMsg, wParam, lParam);
    }
}
