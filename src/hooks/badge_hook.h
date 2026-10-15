#pragma once
#include <cstdint>

namespace Hooks {
    class BadgeHook {
    public:
        static bool Init();
        static void Shutdown();

        static void SetEnabled(bool enabled);
        static bool IsEnabled();

        // Target: 0 = User Team, 1 = User Active Player, 2 = All Players (Arcade)
        static void SetTargetMode(int mode);
        static int GetTargetMode();

        // Tier: 1 = Bronze, 2 = Silver, 3 = Gold (Tier 3)
        static void SetTier(int tier);
        static int GetTier();

    private:
        using CheckAbility_t = int(__fastcall*)(void* pAbility, void* pPlayer);
        using GetAbilityTier_t = int(__fastcall*)(void* pAbility, void* pPlayer);
        using HasSpecialAbility_t = int(__fastcall*)(void* pPlayerAttribs, void* edx, int abilityId);

        static int __fastcall Hooked_CheckPlayerHasAbility(void* pAbility, void* pPlayer);
        static int __fastcall Hooked_GetAbilityTier(void* pAbility, void* pPlayer);
        static int __fastcall Hooked_HasSpecialAbility(void* pPlayerAttribs, void* edx, int abilityId);

        static bool IsPlayerEligible(void* pPlayer);

        static CheckAbility_t s_OriginalCheckPlayerHasAbility;
        static GetAbilityTier_t s_OriginalGetAbilityTier;
        static HasSpecialAbility_t s_OriginalHasSpecialAbility;

        static bool s_Initialized;
        static bool s_Enabled;
        static int s_TargetMode;
        static int s_Tier;
        static uintptr_t s_UserTeamPtr;
    };
}
