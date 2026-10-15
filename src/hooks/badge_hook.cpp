#include "badge_hook.h"
#include "../core/logger.h"
#include <MinHook.h>
#include <windows.h>

namespace Hooks {
    // Engine Function Virtual Addresses
    static constexpr uintptr_t ADDR_CHECK_ABILITY        = 0x006AC270;
    static constexpr uintptr_t ADDR_GET_ABILITY_TIER     = 0x006AC180;
    static constexpr uintptr_t ADDR_HAS_SPECIAL_ABILITY  = 0x00C895A0;

    BadgeHook::CheckAbility_t BadgeHook::s_OriginalCheckPlayerHasAbility = nullptr;
    BadgeHook::GetAbilityTier_t BadgeHook::s_OriginalGetAbilityTier = nullptr;
    BadgeHook::HasSpecialAbility_t BadgeHook::s_OriginalHasSpecialAbility = nullptr;

    bool BadgeHook::s_Initialized = false;
    bool BadgeHook::s_Enabled = false;
    int BadgeHook::s_TargetMode = 0; // 0 = User Team, 1 = Active User Player, 2 = All Players
    int BadgeHook::s_Tier = 3;       // 1 = Bronze, 2 = Silver, 3 = Gold
    uintptr_t BadgeHook::s_UserTeamPtr = 0;

    bool BadgeHook::IsPlayerEligible(void* pPlayer) {
        if (!s_Enabled) return false;
        if (s_TargetMode == 2) {
            // All Players on Court (Arcade Chaos)
            return true;
        }

        if (!pPlayer) return false;

        __try {
            uint32_t slot = *reinterpret_cast<const uint32_t*>((uintptr_t)pPlayer + 0xb0c);
            if (slot > 15) return false;

            // Controller mapping table at 0x01D83328 (each slot = 16 bytes)
            uint32_t controller = *reinterpret_cast<const uint32_t*>(0x01D83328 + (slot << 4));

            if (s_TargetMode == 1) {
                // Active User-Controlled Player Only (P1 = 1, P2 = 2)
                return (controller == 1 || controller == 2);
            } else {
                // User Team Only (TargetMode == 0)
                if (controller == 1 || controller == 2) {
                    uintptr_t teamPtr = *reinterpret_cast<const uintptr_t*>((uintptr_t)pPlayer + 0x4c);
                    if (teamPtr) s_UserTeamPtr = teamPtr;
                    return true;
                }
                if (s_UserTeamPtr != 0) {
                    uintptr_t teamPtr = *reinterpret_cast<const uintptr_t*>((uintptr_t)pPlayer + 0x4c);
                    if (teamPtr == s_UserTeamPtr) return true;
                }
                // Fallback: Slots 1..5 are typically Home team
                return (slot >= 1 && slot <= 5);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    int __fastcall BadgeHook::Hooked_CheckPlayerHasAbility(void* pAbility, void* pPlayer) {
        if (s_Enabled && IsPlayerEligible(pPlayer)) {
            return 1; // Unconditionally grant all 38 Signature Skills
        }
        return s_OriginalCheckPlayerHasAbility(pAbility, pPlayer);
    }

    int __fastcall BadgeHook::Hooked_GetAbilityTier(void* pAbility, void* pPlayer) {
        if (s_Enabled && IsPlayerEligible(pPlayer)) {
            return (s_Tier >= 1 && s_Tier <= 3) ? s_Tier : 3; // Tier 3 Gold Badge
        }
        return s_OriginalGetAbilityTier(pAbility, pPlayer);
    }

    int __fastcall BadgeHook::Hooked_HasSpecialAbility(void* pPlayerAttribs, void* edx, int abilityId) {
        if (s_Enabled) {
            return 1; // Roster & attributes level verification
        }
        return s_OriginalHasSpecialAbility(pPlayerAttribs, edx, abilityId);
    }

    bool BadgeHook::Init() {
        if (s_Initialized) return true;

        LOG_INFO("Installing Signature Skills Master Unlocker hooks (CheckAbility: 0x%08X, Tier: 0x%08X, HasAbility: 0x%08X)...",
            ADDR_CHECK_ABILITY, ADDR_GET_ABILITY_TIER, ADDR_HAS_SPECIAL_ABILITY);

        // Sanity check opcodes
        const unsigned char* pBytes1 = reinterpret_cast<const unsigned char*>(ADDR_CHECK_ABILITY);
        if (pBytes1[0] != 0x56 || pBytes1[1] != 0x8B || pBytes1[2] != 0xF1) {
            LOG_ERROR("BadgeHook ADDR_CHECK_ABILITY validation failed! Found: %02X %02X %02X", pBytes1[0], pBytes1[1], pBytes1[2]);
            return false;
        }

        const unsigned char* pBytes2 = reinterpret_cast<const unsigned char*>(ADDR_GET_ABILITY_TIER);
        if (pBytes2[0] != 0x56 || pBytes2[1] != 0x8B || pBytes2[2] != 0xF1) {
            LOG_ERROR("BadgeHook ADDR_GET_ABILITY_TIER validation failed! Found: %02X %02X %02X", pBytes2[0], pBytes2[1], pBytes2[2]);
            return false;
        }

        MH_STATUS status = MH_CreateHook(
            reinterpret_cast<void*>(ADDR_CHECK_ABILITY),
            reinterpret_cast<void*>(&Hooked_CheckPlayerHasAbility),
            reinterpret_cast<void**>(&s_OriginalCheckPlayerHasAbility)
        );
        if (status != MH_OK) {
            LOG_ERROR("Failed to create hook for CheckPlayerHasAbility! Status: %d", status);
            return false;
        }

        status = MH_CreateHook(
            reinterpret_cast<void*>(ADDR_GET_ABILITY_TIER),
            reinterpret_cast<void*>(&Hooked_GetAbilityTier),
            reinterpret_cast<void**>(&s_OriginalGetAbilityTier)
        );
        if (status != MH_OK) {
            LOG_ERROR("Failed to create hook for GetAbilityTier! Status: %d", status);
            return false;
        }

        status = MH_CreateHook(
            reinterpret_cast<void*>(ADDR_HAS_SPECIAL_ABILITY),
            reinterpret_cast<void*>(&Hooked_HasSpecialAbility),
            reinterpret_cast<void**>(&s_OriginalHasSpecialAbility)
        );
        if (status != MH_OK) {
            LOG_ERROR("Failed to create hook for HasSpecialAbility! Status: %d", status);
            return false;
        }

        MH_EnableHook(reinterpret_cast<void*>(ADDR_CHECK_ABILITY));
        MH_EnableHook(reinterpret_cast<void*>(ADDR_GET_ABILITY_TIER));
        MH_EnableHook(reinterpret_cast<void*>(ADDR_HAS_SPECIAL_ABILITY));

        s_Initialized = true;
        LOG_INFO("Signature Skills Master Unlocker hooks successfully installed.");
        return true;
    }

    void BadgeHook::Shutdown() {
        if (!s_Initialized) return;

        MH_DisableHook(reinterpret_cast<void*>(ADDR_CHECK_ABILITY));
        MH_DisableHook(reinterpret_cast<void*>(ADDR_GET_ABILITY_TIER));
        MH_DisableHook(reinterpret_cast<void*>(ADDR_HAS_SPECIAL_ABILITY));
        s_Initialized = false;
        LOG_INFO("Signature Skills Master Unlocker hooks disabled.");
    }

    void BadgeHook::SetEnabled(bool enabled) {
        s_Enabled = enabled;
        if (!enabled) {
            s_UserTeamPtr = 0;
        }
    }

    bool BadgeHook::IsEnabled() {
        return s_Enabled;
    }

    void BadgeHook::SetTargetMode(int mode) {
        s_TargetMode = mode;
        s_UserTeamPtr = 0;
    }

    int BadgeHook::GetTargetMode() {
        return s_TargetMode;
    }

    void BadgeHook::SetTier(int tier) {
        s_Tier = tier;
    }

    int BadgeHook::GetTier() {
        return s_Tier;
    }
}
