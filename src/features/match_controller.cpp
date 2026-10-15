#include "match_controller.h"
#include "../core/logger.h"
#include "feature_manager.h"
#include <windows.h>
#include <algorithm>

namespace Features {
    // Verified fixed static memory addresses in nba2k14.exe (.data section)
    static constexpr uintptr_t ADDR_SHOT_CLOCK    = 0x01dee638; // Current Shot Clock float
    static constexpr uintptr_t ADDR_GAME_CLOCK    = 0x01dee654; // Current Quarter Game Clock float
    static constexpr uintptr_t ADDR_USER_FATIGUE  = 0x01e6965c; // User Fatigue Rate slider float
    static constexpr uintptr_t ADDR_CPU_FATIGUE   = 0x01e69660; // CPU Fatigue Rate slider float

    MatchController& MatchController::Get() {
        static MatchController instance;
        return instance;
    }

    void MatchController::Init() {
        LOG_INFO("Match Controller initialized (ShotClock: 0x%08X, GameClock: 0x%08X, Fatigue: 0x%08X)",
            ADDR_SHOT_CLOCK, ADDR_GAME_CLOCK, ADDR_USER_FATIGUE);
    }

    bool MatchController::IsInMatch() {
        __try {
            float gClock = *reinterpret_cast<const float*>(ADDR_GAME_CLOCK);
            float sClock = *reinterpret_cast<const float*>(ADDR_SHOT_CLOCK);
            return (gClock > 0.01f && gClock < 3600.0f) || (sClock > 0.01f && sClock <= 35.0f);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    float MatchController::GetShotClock() {
        __try {
            return *reinterpret_cast<const float*>(ADDR_SHOT_CLOCK);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0.0f;
        }
    }

    void MatchController::SetShotClock(float seconds) {
        __try {
            *reinterpret_cast<float*>(ADDR_SHOT_CLOCK) = (std::max)(0.0f, seconds);
            m_FrozenShotClockValue = seconds;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void MatchController::ResetShotClock24() {
        SetShotClock(24.0f);
        LOG_INFO("[MatchController] Shot clock reset to 24.0s");
    }

    void MatchController::ResetShotClock14() {
        SetShotClock(14.0f);
        LOG_INFO("[MatchController] Shot clock reset to 14.0s");
    }

    void MatchController::SetShotClockPressure() {
        SetShotClock(2.0f);
        LOG_INFO("[MatchController] Shot clock set to 2.0s (Buzzer Beater Mode)");
    }

    void MatchController::SetFreezeShotClock(bool freeze) {
        m_FreezeShotClock = freeze;
        if (freeze) {
            m_FrozenShotClockValue = GetShotClock();
            if (m_FrozenShotClockValue <= 0.5f) m_FrozenShotClockValue = 24.0f;
            LOG_INFO("[MatchController] Shot clock frozen at %.1fs", m_FrozenShotClockValue);
        } else {
            LOG_INFO("[MatchController] Shot clock unfrozen");
        }
    }

    float MatchController::GetGameClock() {
        __try {
            return *reinterpret_cast<const float*>(ADDR_GAME_CLOCK);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0.0f;
        }
    }

    void MatchController::SetGameClock(float seconds) {
        __try {
            *reinterpret_cast<float*>(ADDR_GAME_CLOCK) = (std::max)(0.0f, seconds);
            m_FrozenGameClockValue = seconds;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void MatchController::AddGameClock(float seconds) {
        float current = GetGameClock();
        SetGameClock((std::max)(0.0f, current + seconds));
    }

    void MatchController::InstantEndOfQuarter() {
        __try {
            *reinterpret_cast<float*>(ADDR_GAME_CLOCK) = 0.0f;
            *reinterpret_cast<float*>(ADDR_SHOT_CLOCK) = 0.0f;
            LOG_INFO("[MatchController] Quarter ended instantly via game clock override");
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void MatchController::SetFreezeGameClock(bool freeze) {
        m_FreezeGameClock = freeze;
        if (freeze) {
            m_FrozenGameClockValue = GetGameClock();
            LOG_INFO("[MatchController] Game clock frozen at %.1fs", m_FrozenGameClockValue);
        } else {
            LOG_INFO("[MatchController] Game clock unfrozen");
        }
    }

    void MatchController::SetUnlimitedStamina(bool enabled) {
        m_UnlimitedStamina = enabled;
        __try {
            if (enabled) {
                if (!m_FatigueSaved) {
                    m_SavedUserFatigueRate = *reinterpret_cast<const float*>(ADDR_USER_FATIGUE);
                    m_SavedCpuFatigueRate = *reinterpret_cast<const float*>(ADDR_CPU_FATIGUE);
                    m_FatigueSaved = true;
                }
                *reinterpret_cast<float*>(ADDR_USER_FATIGUE) = 0.0f;
                *reinterpret_cast<float*>(ADDR_CPU_FATIGUE) = 0.0f;
                LOG_INFO("[MatchController] Unlimited Stamina ENABLED (Fatigue Rate set to 0.00)");
            } else {
                if (m_FatigueSaved) {
                    *reinterpret_cast<float*>(ADDR_USER_FATIGUE) = m_SavedUserFatigueRate;
                    *reinterpret_cast<float*>(ADDR_CPU_FATIGUE) = m_SavedCpuFatigueRate;
                } else {
                    *reinterpret_cast<float*>(ADDR_USER_FATIGUE) = 0.5f;
                    *reinterpret_cast<float*>(ADDR_CPU_FATIGUE) = 0.5f;
                }
                LOG_INFO("[MatchController] Unlimited Stamina DISABLED (Fatigue Rate restored to %.2f)", m_SavedUserFatigueRate);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void MatchController::Update() {
        auto& settings = FeatureManager::Get().GetSettings();

        // Synchronize settings with controller state
        if (settings.unlimitedStamina != m_UnlimitedStamina) {
            SetUnlimitedStamina(settings.unlimitedStamina);
        }

        // Active Clock Freezing
        if (m_FreezeShotClock) {
            __try {
                *reinterpret_cast<float*>(ADDR_SHOT_CLOCK) = m_FrozenShotClockValue;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        if (m_FreezeGameClock) {
            __try {
                *reinterpret_cast<float*>(ADDR_GAME_CLOCK) = m_FrozenGameClockValue;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        if (m_UnlimitedStamina) {
            __try {
                *reinterpret_cast<float*>(ADDR_USER_FATIGUE) = 0.0f;
                *reinterpret_cast<float*>(ADDR_CPU_FATIGUE) = 0.0f;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        // Handle in-game hotkeys when overlay menu is closed
        if (!settings.showMenu) {
            // F9: Reset Shot Clock to 24s
            bool f9Down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
            if (f9Down && !m_PrevF9Down) {
                ResetShotClock24();
            }
            m_PrevF9Down = f9Down;

            // F10: Reset Shot Clock to 14s
            bool f10Down = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
            if (f10Down && !m_PrevF10Down) {
                ResetShotClock14();
            }
            m_PrevF10Down = f10Down;

            // F11: Toggle Freeze Shot Clock
            bool f11Down = (GetAsyncKeyState(VK_F11) & 0x8000) != 0;
            if (f11Down && !m_PrevF11Down) {
                SetFreezeShotClock(!m_FreezeShotClock);
            }
            m_PrevF11Down = f11Down;

            // F12: Instant End of Quarter
            bool f12Down = (GetAsyncKeyState(VK_F12) & 0x8000) != 0;
            if (f12Down && !m_PrevF12Down) {
                InstantEndOfQuarter();
            }
            m_PrevF12Down = f12Down;
        }
    }
}
