#include "d3d9_hook.h"
#include "wndproc_hook.h"
#include "../ui/menu.h"
#include "../core/logger.h"
#include "../features/feature_manager.h"
#include <MinHook.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

namespace Hooks {
    D3D9Hook::EndScene_t D3D9Hook::s_OriginalEndScene = nullptr;
    D3D9Hook::Reset_t D3D9Hook::s_OriginalReset = nullptr;
    void* D3D9Hook::s_TargetEndScene = nullptr;
    void* D3D9Hook::s_TargetReset = nullptr;
    bool D3D9Hook::s_Initialized = false;

    static HWND GetProcessWindow() {
        HWND hwnd = nullptr;
        EnumWindows([](HWND top, LPARAM lparam) -> BOOL {
            DWORD pid = 0;
            GetWindowThreadProcessId(top, &pid);
            if (pid == GetCurrentProcessId() && IsWindowVisible(top)) {
                *reinterpret_cast<HWND*>(lparam) = top;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&hwnd));
        return hwnd;
    }

    bool D3D9Hook::Init() {
        LOG_INFO("Creating dummy Direct3D9 device to locate VTable...");

        WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_CLASSDC, DefWindowProcA, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "NBA2K14_DUMMY_WC", NULL };
        RegisterClassExA(&wc);
        HWND dummyHwnd = CreateWindowA("NBA2K14_DUMMY_WC", "DUMMY", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);

        IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
        if (!pD3D) {
            LOG_ERROR("Failed to create Direct3D9 interface!");
            DestroyWindow(dummyHwnd);
            UnregisterClassA(wc.lpszClassName, wc.hInstance);
            return false;
        }

        D3DPRESENT_PARAMETERS d3dpp = {};
        d3dpp.Windowed = TRUE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.hDeviceWindow = dummyHwnd;

        IDirect3DDevice9* pDummyDevice = nullptr;
        HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dummyHwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
        if (FAILED(hr) || !pDummyDevice) {
            LOG_ERROR("Failed to create dummy D3D9 device (0x%08X)!", hr);
            pD3D->Release();
            DestroyWindow(dummyHwnd);
            UnregisterClassA(wc.lpszClassName, wc.hInstance);
            return false;
        }

        void** vtable = *reinterpret_cast<void***>(pDummyDevice);
        s_TargetReset = vtable[16];
        s_TargetEndScene = vtable[42];

        LOG_INFO("DirectX 9 VTable found: Reset=0x%p, EndScene=0x%p", s_TargetReset, s_TargetEndScene);

        pDummyDevice->Release();
        pD3D->Release();
        DestroyWindow(dummyHwnd);
        UnregisterClassA(wc.lpszClassName, wc.hInstance);

        if (MH_Initialize() != MH_OK) {
            LOG_ERROR("Failed to initialize MinHook!");
            return false;
        }

        if (MH_CreateHook(s_TargetEndScene, &Hooked_EndScene, reinterpret_cast<void**>(&s_OriginalEndScene)) != MH_OK) {
            LOG_ERROR("Failed to create hook for EndScene!");
            return false;
        }

        if (MH_CreateHook(s_TargetReset, &Hooked_Reset, reinterpret_cast<void**>(&s_OriginalReset)) != MH_OK) {
            LOG_ERROR("Failed to create hook for Reset!");
            return false;
        }

        if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
            LOG_ERROR("Failed to enable MinHook hooks!");
            return false;
        }

        LOG_INFO("DirectX 9 hooks successfully installed!");
        return true;
    }

    void D3D9Hook::Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        UI::Menu::Shutdown();
        WndProcHook::Shutdown();
        LOG_INFO("DirectX 9 hooks uninstalled.");
    }

    HRESULT WINAPI D3D9Hook::Hooked_EndScene(IDirect3DDevice9* pDevice) {
        if (!s_Initialized) {
            D3DDEVICE_CREATION_PARAMETERS cp;
            pDevice->GetCreationParameters(&cp);
            HWND gameHwnd = cp.hFocusWindow;
            if (!gameHwnd) {
                gameHwnd = GetProcessWindow();
            }

            if (gameHwnd) {
                UI::Menu::Init(pDevice, gameHwnd);
                WndProcHook::Init(gameHwnd);
                s_Initialized = true;
                LOG_INFO("DirectX 9 Game Overlay initialized on HWND: 0x%p", gameHwnd);
            }
        }

        if (s_Initialized) {
            // Update features
            Features::FeatureManager::Get().Update();

            // Render ImGui overlay
            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            UI::Menu::Render();

            ImGui::EndFrame();
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        }

        return s_OriginalEndScene(pDevice);
    }

    HRESULT WINAPI D3D9Hook::Hooked_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
        if (s_Initialized) {
            ImGui_ImplDX9_InvalidateDeviceObjects();
        }

        HRESULT hr = s_OriginalReset(pDevice, pPresentationParameters);

        if (s_Initialized && SUCCEEDED(hr)) {
            ImGui_ImplDX9_CreateDeviceObjects();
        }

        return hr;
    }
}
