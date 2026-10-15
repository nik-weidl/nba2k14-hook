#include "camera_hook.h"
#include "../core/logger.h"
#include <MinHook.h>
#include <windows.h>
#include <cmath>

namespace Hooks {

    // 0x00452490: CAMERA_LENS::Update(CAMERA_LENS* this, void* edx, CAMERA_VIEWPORT* pCam, ...)
    static constexpr uintptr_t ADDR_CAMERA_LENS_UPDATE = 0x00452490;

    // VCFEATURECODE / Debug Display variable in .data
    static constexpr uintptr_t ADDR_VC_DEBUG_OVERLAY   = 0x01E85C80;

    CameraHook::CameraLensUpdate_t CameraHook::s_OriginalCameraLensUpdate = nullptr;
    bool CameraHook::s_Initialized = false;
    bool CameraHook::s_FovOverride = false;
    float CameraHook::s_FovDegrees = 60.0f;
    bool CameraHook::s_EngineDebugOverlay = false;

    bool CameraHook::Init() {
        if (s_Initialized) return true;

        LOG_INFO("[CameraHook] Initializing camera lens and FOV hook at 0x%08X...", ADDR_CAMERA_LENS_UPDATE);

        MH_STATUS status = MH_CreateHook(
            reinterpret_cast<LPVOID>(ADDR_CAMERA_LENS_UPDATE),
            reinterpret_cast<LPVOID>(&CameraHook::Hooked_CameraLensUpdate),
            reinterpret_cast<LPVOID*>(&s_OriginalCameraLensUpdate)
        );

        if (status != MH_OK) {
            LOG_ERROR("[CameraHook] Failed to create hook for CAMERA_LENS::Update: %s", MH_StatusToString(status));
            return false;
        }

        status = MH_EnableHook(reinterpret_cast<LPVOID>(ADDR_CAMERA_LENS_UPDATE));
        if (status != MH_OK) {
            LOG_ERROR("[CameraHook] Failed to enable hook for CAMERA_LENS::Update: %s", MH_StatusToString(status));
            return false;
        }

        s_Initialized = true;
        LOG_INFO("[CameraHook] Camera lens hook initialized and enabled successfully.");
        return true;
    }

    void CameraHook::Shutdown() {
        if (!s_Initialized) return;

        MH_DisableHook(reinterpret_cast<LPVOID>(ADDR_CAMERA_LENS_UPDATE));
        s_Initialized = false;
        LOG_INFO("[CameraHook] Camera hook disabled and shutdown.");
    }

    void CameraHook::SetFovOverride(bool enabled) {
        s_FovOverride = enabled;
        LOG_INFO("[CameraHook] Dynamic FOV Override: %s (Target FOV: %.1f deg)",
            enabled ? "ENABLED" : "DISABLED", s_FovDegrees);
    }

    bool CameraHook::IsFovOverride() {
        return s_FovOverride;
    }

    void CameraHook::SetFovDegrees(float fovDeg) {
        if (fovDeg < 25.0f) fovDeg = 25.0f;
        if (fovDeg > 130.0f) fovDeg = 130.0f;
        s_FovDegrees = fovDeg;
    }

    float CameraHook::GetFovDegrees() {
        return s_FovDegrees;
    }

    void CameraHook::SetEngineDebugOverlay(bool enabled) {
        s_EngineDebugOverlay = enabled;
        __try {
            *reinterpret_cast<uint32_t*>(ADDR_VC_DEBUG_OVERLAY) = enabled ? 1 : 0;
            LOG_INFO("[CameraHook] Engine Debug Overlay: %s", enabled ? "ENABLED" : "DISABLED");
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    bool CameraHook::IsEngineDebugOverlay() {
        return s_EngineDebugOverlay;
    }

    void __fastcall CameraHook::Hooked_CameraLensUpdate(void* pLens, void* edx, void* arg1, void* arg2) {
        if (s_OriginalCameraLensUpdate) {
            s_OriginalCameraLensUpdate(pLens, edx, arg1, arg2);
        }

        __try {
            if (s_FovOverride && pLens != nullptr) {
                // NBA 2K14 stores horizontal FOV in radians at [pLens + 0xa4]
                // 1 radian = ~57.2957795 degrees
                float radH = s_FovDegrees * (3.14159265358979323846f / 180.0f);
                *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(pLens) + 0xa4) = radH;

                // NBA 2K14 stores vertical FOV in radians at [pLens + 0xa8]
                // Preserve standard 16:9 aspect ratio scaling: radV = radH / (16/9)
                float radV = radH * (9.0f / 16.0f);
                *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(pLens) + 0xa8) = radV;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

}
