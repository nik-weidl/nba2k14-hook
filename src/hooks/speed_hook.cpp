#include "speed_hook.h"
#include "../core/logger.h"
#include <MinHook.h>
#include <algorithm>

namespace Hooks {
    SpeedHook::Engine_GetTickCount_t SpeedHook::s_OriginalEngine_GetTickCount = nullptr;
    void* SpeedHook::s_TargetFunction = reinterpret_cast<void*>(0x00D96740);
    bool SpeedHook::s_Initialized = false;
    bool SpeedHook::s_Enabled = false;
    float SpeedHook::s_SpeedMultiplier = 1.0f;
    float SpeedHook::s_EffectiveSpeed = 1.0f;
    bool SpeedHook::s_BulletTimeActive = false;

    uint64_t SpeedHook::s_LastRealTicks = 0;
    uint64_t SpeedHook::s_VirtualTicks = 0;
    SRWLOCK SpeedHook::s_Lock = SRWLOCK_INIT;

    bool SpeedHook::Init() {
        if (s_Initialized) return true;

        LOG_INFO("Installing Game Speed Engine hook (Tick Counter at 0x%p)...", s_TargetFunction);

        // Sanity check: Ensure memory matches expected x86 opcode (0x83 0xEC 0x08 = sub esp, 8)
        unsigned char* pBytes = reinterpret_cast<unsigned char*>(s_TargetFunction);
        if (pBytes[0] != 0x83 || pBytes[1] != 0xEC || pBytes[2] != 0x08) {
            LOG_ERROR("SpeedHook target validation failed! Unexpected opcodes at 0x%p: %02X %02X %02X",
                s_TargetFunction, pBytes[0], pBytes[1], pBytes[2]);
            return false;
        }

        MH_STATUS status = MH_CreateHook(
            s_TargetFunction,
            reinterpret_cast<void*>(&Hooked_Engine_GetTickCount),
            reinterpret_cast<void**>(&s_OriginalEngine_GetTickCount)
        );

        if (status != MH_OK) {
            LOG_ERROR("Failed to create hook for Engine_GetTickCount! Status: %d", status);
            return false;
        }

        status = MH_EnableHook(s_TargetFunction);
        if (status != MH_OK) {
            LOG_ERROR("Failed to enable hook for Engine_GetTickCount! Status: %d", status);
            return false;
        }

        s_Initialized = true;
        LOG_INFO("Game Speed Engine successfully initialized and active.");
        return true;
    }

    void SpeedHook::Shutdown() {
        if (!s_Initialized) return;

        if (s_TargetFunction) {
            MH_DisableHook(s_TargetFunction);
        }

        s_Initialized = false;
        LOG_INFO("Game Speed Engine hook uninstalled.");
    }

    void SpeedHook::SetEnabled(bool enabled) {
        s_Enabled = enabled;
    }

    bool SpeedHook::IsEnabled() {
        return s_Enabled;
    }

    void SpeedHook::SetSpeed(float multiplier) {
        s_SpeedMultiplier = (std::max)(0.0f, (std::min)(multiplier, 10.0f));
    }

    float SpeedHook::GetSpeed() {
        return s_SpeedMultiplier;
    }

    void SpeedHook::SetEffectiveSpeed(float multiplier) {
        s_EffectiveSpeed = (std::max)(0.0f, (std::min)(multiplier, 10.0f));
    }

    float SpeedHook::GetEffectiveSpeed() {
        return s_EffectiveSpeed;
    }

    bool SpeedHook::IsBulletTimeActive() {
        return s_BulletTimeActive;
    }

    void SpeedHook::SetBulletTimeActive(bool active) {
        s_BulletTimeActive = active;
    }

    uint64_t __cdecl SpeedHook::Hooked_Engine_GetTickCount() {
        if (!s_OriginalEngine_GetTickCount) {
            return 0;
        }

        uint64_t realTicks = s_OriginalEngine_GetTickCount();

        AcquireSRWLockExclusive(&s_Lock);

        if (s_LastRealTicks == 0) {
            s_LastRealTicks = realTicks;
            s_VirtualTicks = realTicks;
            ReleaseSRWLockExclusive(&s_Lock);
            return realTicks;
        }

        uint64_t delta = (realTicks >= s_LastRealTicks) ? (realTicks - s_LastRealTicks) : 0;
        s_LastRealTicks = realTicks;

        // Protection against extreme jumps (e.g., system pause, hibernation, or debugger pause)
        // 50,000,000 ticks is approx 5.0s on standard 10MHz QPC
        if (delta > 50000000ULL) {
            delta = 50000000ULL;
        }

        float activeMultiplier = s_Enabled ? s_EffectiveSpeed : 1.0f;
        if (activeMultiplier < 0.0f) activeMultiplier = 0.0f;

        s_VirtualTicks += static_cast<uint64_t>(static_cast<double>(delta) * static_cast<double>(activeMultiplier));
        uint64_t result = s_VirtualTicks;

        ReleaseSRWLockExclusive(&s_Lock);
        return result;
    }
}
