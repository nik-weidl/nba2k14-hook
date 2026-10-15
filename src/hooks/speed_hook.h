#pragma once
#include <cstdint>
#include <windows.h>

namespace Hooks {
    class SpeedHook {
    public:
        static bool Init();
        static void Shutdown();

        static void SetEnabled(bool enabled);
        static bool IsEnabled();

        static void SetSpeed(float multiplier);
        static float GetSpeed();

        static void SetEffectiveSpeed(float multiplier);
        static float GetEffectiveSpeed();

        static bool IsBulletTimeActive();
        static void SetBulletTimeActive(bool active);

    private:
        using Engine_GetTickCount_t = uint64_t(__cdecl*)();
        static Engine_GetTickCount_t s_OriginalEngine_GetTickCount;
        static uint64_t __cdecl Hooked_Engine_GetTickCount();

        static void* s_TargetFunction;
        static bool s_Initialized;
        static bool s_Enabled;
        static float s_SpeedMultiplier;
        static float s_EffectiveSpeed;
        static bool s_BulletTimeActive;

        static uint64_t s_LastRealTicks;
        static uint64_t s_VirtualTicks;
        static SRWLOCK s_Lock;
    };
}
