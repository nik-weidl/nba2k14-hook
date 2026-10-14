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
        DWORD currentPid = GetCurrentProcessId();

        EnumWindows([](HWND top, LPARAM lparam) -> BOOL {
            DWORD pid = 0;
            GetWindowThreadProcessId(top, &pid);
            if (pid == *reinterpret_cast<DWORD*>(lparam) && IsWindowVisible(top)) {
                char title[256];
                GetWindowTextA(top, title, sizeof(title));
                // Prefer NBA 2K14 main window
                if (strstr(title, "NBA") || strstr(title, "2K") || strlen(title) > 0) {
                    *reinterpret_cast<HWND*>(lparam) = top;
                    return FALSE;
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&currentPid));

        if (currentPid != GetCurrentProcessId()) {
            return (HWND)currentPid;
        }

        return GetActiveWindow();
    }

    bool D3D9Hook::Init() {
        LOG_INFO("Initializing DirectX 9 hook via clean System32 runtime...");

        // Always load clean system d3d9.dll explicitly to prevent conflicts with local ENB/ReShade
        HMODULE hD3D9 = LoadLibraryA("C:\\Windows\\System32\\d3d9.dll");
        if (!hD3D9) {
            LOG_ERROR("Failed to load C:\\Windows\\System32\\d3d9.dll!");
            return false;
        }

        using Direct3DCreate9Ex_t = HRESULT(WINAPI*)(UINT, IDirect3D9Ex**);
        Direct3DCreate9Ex_t pDirect3DCreate9Ex = (Direct3DCreate9Ex_t)GetProcAddress(hD3D9, "Direct3DCreate9Ex");

        WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_CLASSDC, DefWindowProcA, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "NBA2K14_HOOK_DUMMY", NULL };
        RegisterClassExA(&wc);
        HWND dummyHwnd = CreateWindowA("NBA2K14_HOOK_DUMMY", "DUMMY", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);

        if (!dummyHwnd) {
            LOG_ERROR("Failed to create dummy window! Error: %lu", GetLastError());
            return false;
        }

        void** vtable = nullptr;

        // Modern Windows 10/11 & NVIDIA path: Direct3DCreate9Ex
        if (pDirect3DCreate9Ex) {
            IDirect3D9Ex* pD3DEx = nullptr;
            HRESULT hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &pD3DEx);
            if (SUCCEEDED(hr) && pD3DEx) {
                D3DDISPLAYMODEEX dm = { sizeof(D3DDISPLAYMODEEX) };
                pD3DEx->GetAdapterDisplayModeEx(0, &dm, NULL);

                D3DPRESENT_PARAMETERS d3dpp = {};
                d3dpp.Windowed = TRUE;
                d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
                d3dpp.hDeviceWindow = dummyHwnd;
                d3dpp.BackBufferWidth = 100;
                d3dpp.BackBufferHeight = 100;
                d3dpp.BackBufferFormat = (dm.Format != D3DFMT_UNKNOWN) ? dm.Format : D3DFMT_X8R8G8B8;

                IDirect3DDevice9Ex* pDeviceEx = nullptr;
                hr = pD3DEx->CreateDeviceEx(0, D3DDEVTYPE_HAL, dummyHwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_NOWINDOWCHANGES, &d3dpp, NULL, &pDeviceEx);
                if (SUCCEEDED(hr) && pDeviceEx) {
                    vtable = *reinterpret_cast<void***>(pDeviceEx);
                    s_TargetReset = vtable[16];
                    s_TargetEndScene = vtable[42];
                    LOG_INFO("VTable obtained via Direct3DCreate9Ex: Reset=0x%p, EndScene=0x%p", s_TargetReset, s_TargetEndScene);
                    pDeviceEx->Release();
                } else {
                    LOG_WARN("CreateDeviceEx failed (0x%08X). Trying legacy path...", hr);
                }
                pD3DEx->Release();
            }
        }

        // Fallback for legacy systems: Direct3DCreate9
        if (!s_TargetEndScene) {
            using Direct3DCreate9_t = IDirect3D9*(WINAPI*)(UINT);
            Direct3DCreate9_t pDirect3DCreate9 = (Direct3DCreate9_t)GetProcAddress(hD3D9, "Direct3DCreate9");
            if (pDirect3DCreate9) {
                IDirect3D9* pD3D = pDirect3DCreate9(D3D_SDK_VERSION);
                if (pD3D) {
                    D3DDISPLAYMODE dm = {};
                    pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dm);

                    D3DPRESENT_PARAMETERS d3dpp = {};
                    d3dpp.Windowed = TRUE;
                    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
                    d3dpp.hDeviceWindow = dummyHwnd;
                    d3dpp.BackBufferFormat = (dm.Format != D3DFMT_UNKNOWN) ? dm.Format : D3DFMT_X8R8G8B8;

                    IDirect3DDevice9* pDummyDevice = nullptr;
                    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dummyHwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_NOWINDOWCHANGES, &d3dpp, &pDummyDevice);
                    if (SUCCEEDED(hr) && pDummyDevice) {
                        vtable = *reinterpret_cast<void***>(pDummyDevice);
                        s_TargetReset = vtable[16];
                        s_TargetEndScene = vtable[42];
                        LOG_INFO("VTable obtained via Direct3DCreate9: Reset=0x%p, EndScene=0x%p", s_TargetReset, s_TargetEndScene);
                        pDummyDevice->Release();
                    }
                    pD3D->Release();
                }
            }
        }

        DestroyWindow(dummyHwnd);
        UnregisterClassA("NBA2K14_HOOK_DUMMY", wc.hInstance);

        if (!s_TargetEndScene || !s_TargetReset) {
            LOG_ERROR("Could not locate DirectX 9 VTable functions!");
            return false;
        }

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
        if (!pDevice) return s_OriginalEndScene(pDevice);

        if (!s_Initialized) {
            D3DDEVICE_CREATION_PARAMETERS cp = {};
            HWND gameHwnd = nullptr;
            if (SUCCEEDED(pDevice->GetCreationParameters(&cp)) && cp.hFocusWindow) {
                gameHwnd = cp.hFocusWindow;
            }
            if (!gameHwnd) {
                gameHwnd = GetProcessWindow();
            }

            if (gameHwnd && IsWindow(gameHwnd)) {
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
