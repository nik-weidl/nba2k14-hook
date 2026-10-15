#pragma once
#include <cstdint>

namespace Hooks {
    class CameraHook {
    public:
        static bool Init();
        static void Shutdown();

        static void SetFovOverride(bool enabled);
        static bool IsFovOverride();

        static void SetFovDegrees(float fovDeg);
        static float GetFovDegrees();

        static void SetEngineDebugOverlay(bool enabled);
        static bool IsEngineDebugOverlay();

    private:
        using CameraLensUpdate_t = void(__fastcall*)(void* pLens, void* edx, void* arg1, void* arg2);

        static void __fastcall Hooked_CameraLensUpdate(void* pLens, void* edx, void* arg1, void* arg2);

        static CameraLensUpdate_t s_OriginalCameraLensUpdate;
        static bool s_Initialized;
        static bool s_FovOverride;
        static float s_FovDegrees;
        static bool s_EngineDebugOverlay;
    };
}
