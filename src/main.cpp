#include <windows.h>
#include "core/logger.h"
#include "hooks/d3d9_hook.h"
#include "hooks/file_hook.h"
#include "features/feature_manager.h"
#include "features/vfs.h"
#include <MinHook.h>

// Original DirectInput8Create function pointer
using DirectInput8Create_t = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
static DirectInput8Create_t s_OriginalDirectInput8Create = nullptr;
static HMODULE s_OriginalDInputModule = nullptr;

// Exported DirectInput8 proxy forwarder
extern "C" HRESULT WINAPI DirectInput8Create_Proxy(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter) {
    if (!s_OriginalDirectInput8Create) {
        char systemPath[MAX_PATH];
        GetSystemDirectoryA(systemPath, MAX_PATH);
        strcat_s(systemPath, "\\dinput8.dll");

        s_OriginalDInputModule = LoadLibraryA(systemPath);
        if (s_OriginalDInputModule) {
            s_OriginalDirectInput8Create = (DirectInput8Create_t)GetProcAddress(s_OriginalDInputModule, "DirectInput8Create");
            LOG_INFO("Loaded system dinput8.dll from %s (proc: 0x%p)", systemPath, s_OriginalDirectInput8Create);
        } else {
            LOG_ERROR("Failed to load real system dinput8.dll from %s!", systemPath);
        }
    }

    if (s_OriginalDirectInput8Create) {
        return s_OriginalDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    }

    return E_FAIL;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    LOG_INFO("NBA 2K14 Hook background thread started.");

    // Initialize VFS and File Hooks early so all game archives can be redirected
    Features::VFS::Get().Init();
    Hooks::FileHook::Init();

    // Brief delay to allow the game window and graphics subsystem to reach stable state
    Sleep(1200);

    Features::FeatureManager::Get().Init();

    if (!Hooks::D3D9Hook::Init()) {
        LOG_ERROR("DirectX 9 Hook initialization failed!");
    } else {
        LOG_INFO("NBA 2K14 Hook initialization complete. Press [F8] in-game to toggle menu!");
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        Core::Logger::Init();
        LOG_INFO("DLL_PROCESS_ATTACH: Starting NBA 2K14 Native Hook...");
        MH_Initialize();
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        LOG_INFO("DLL_PROCESS_DETACH: Cleaning up...");
        Hooks::FileHook::Shutdown();
        Hooks::D3D9Hook::Shutdown();
        MH_Uninitialize();
        Core::Logger::Shutdown();
        if (s_OriginalDInputModule) {
            FreeLibrary(s_OriginalDInputModule);
            s_OriginalDInputModule = nullptr;
        }
        break;
    }
    return TRUE;
}
