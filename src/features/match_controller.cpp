#include "match_controller.h"
#include "../core/logger.h"
#include "feature_manager.h"
#include <windows.h>
#include <algorithm>

namespace Features {
    // Verified fixed static memory addresses in nba2k14.exe (.data section)
    static constexpr uintptr_t ADDR_SHOT_CLOCK        = 0x01dee638; // Current Shot Clock float
    static constexpr uintptr_t ADDR_GAME_CLOCK        = 0x01dee654; // Current Quarter Game Clock float
    static constexpr uintptr_t ADDR_USER_FATIGUE      = 0x01e6965c; // User Fatigue Rate slider float
    static constexpr uintptr_t ADDR_CPU_FATIGUE       = 0x01e69660; // CPU Fatigue Rate slider float

    // Shooting, Dunk & Defense Sliders
    static constexpr uintptr_t ADDR_SLIDER_TABLE      = 0x01e69560;
    static constexpr uintptr_t ADDR_SHOT_INSIDE_USER   = ADDR_SLIDER_TABLE + 0x04;
    static constexpr uintptr_t ADDR_SHOT_CLOSE_USER    = ADDR_SLIDER_TABLE + 0x0c;
    static constexpr uintptr_t ADDR_SHOT_MID_USER      = ADDR_SLIDER_TABLE + 0x14;
    static constexpr uintptr_t ADDR_SHOT_3PT_USER      = ADDR_SLIDER_TABLE + 0x1c;
    static constexpr uintptr_t ADDR_SHOT_LAYUP_USER    = ADDR_SLIDER_TABLE + 0x24;
    static constexpr uintptr_t ADDR_DUNK_TRAFFIC_FREQ  = ADDR_SLIDER_TABLE + 0x2c;
    static constexpr uintptr_t ADDR_DUNK_TRAFFIC_SUCC  = ADDR_SLIDER_TABLE + 0x34;

    // Foul Sliders (User)
    static constexpr uintptr_t ADDR_CHARGING_FOUL      = ADDR_SLIDER_TABLE + 0x16c;
    static constexpr uintptr_t ADDR_BLOCKING_FOUL      = ADDR_SLIDER_TABLE + 0x174;
    static constexpr uintptr_t ADDR_REACHING_FOUL      = ADDR_SLIDER_TABLE + 0x194;
    static constexpr uintptr_t ADDR_SHOOTING_FOUL      = ADDR_SLIDER_TABLE + 0x19c;
    static constexpr uintptr_t ADDR_LOOSE_BALL_FOUL    = ADDR_SLIDER_TABLE + 0x1a4;

    // Late Game Intentional Fouling (VCFEATURECODE)
    static constexpr uintptr_t ADDR_DISABLE_LATE_GAME_FOULING_TEAM0 = 0x01e85c9c;
    static constexpr uintptr_t ADDR_DISABLE_LATE_GAME_FOULING_TEAM1 = 0x01e85ca0;

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

    void MatchController::SetGodShooting(bool enabled) {
        m_GodShooting = enabled;
        __try {
            if (enabled) {
                if (!m_SlidersSaved) {
                    m_SavedInsideShot = *reinterpret_cast<const float*>(ADDR_SHOT_INSIDE_USER);
                    m_SavedCloseShot  = *reinterpret_cast<const float*>(ADDR_SHOT_CLOSE_USER);
                    m_SavedMidShot    = *reinterpret_cast<const float*>(ADDR_SHOT_MID_USER);
                    m_Saved3ptShot    = *reinterpret_cast<const float*>(ADDR_SHOT_3PT_USER);
                    m_SavedLayup      = *reinterpret_cast<const float*>(ADDR_SHOT_LAYUP_USER);
                    m_SlidersSaved = true;
                }
                *reinterpret_cast<float*>(ADDR_SHOT_INSIDE_USER) = 1.0f;
                *reinterpret_cast<float*>(ADDR_SHOT_CLOSE_USER)  = 1.0f;
                *reinterpret_cast<float*>(ADDR_SHOT_MID_USER)    = 1.0f;
                *reinterpret_cast<float*>(ADDR_SHOT_3PT_USER)    = 1.0f;
                *reinterpret_cast<float*>(ADDR_SHOT_LAYUP_USER)  = 1.0f;
                LOG_INFO("[MatchController] God Shooting Assist ENABLED (100%% Shot Make Rates)");
            } else {
                if (m_SlidersSaved) {
                    *reinterpret_cast<float*>(ADDR_SHOT_INSIDE_USER) = m_SavedInsideShot;
                    *reinterpret_cast<float*>(ADDR_SHOT_CLOSE_USER)  = m_SavedCloseShot;
                    *reinterpret_cast<float*>(ADDR_SHOT_MID_USER)    = m_SavedMidShot;
                    *reinterpret_cast<float*>(ADDR_SHOT_3PT_USER)    = m_Saved3ptShot;
                    *reinterpret_cast<float*>(ADDR_SHOT_LAYUP_USER)  = m_SavedLayup;
                }
                LOG_INFO("[MatchController] God Shooting Assist DISABLED");
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void MatchController::SetHighlightDunkBoost(bool enabled) {
        m_HighlightDunkBoost = enabled;
        __try {
            if (enabled) {
                m_SavedDunkFreq = *reinterpret_cast<const float*>(ADDR_DUNK_TRAFFIC_FREQ);
                m_SavedDunkSucc = *reinterpret_cast<const float*>(ADDR_DUNK_TRAFFIC_SUCC);
                *reinterpret_cast<float*>(ADDR_DUNK_TRAFFIC_FREQ) = 1.0f;
                *reinterpret_cast<float*>(ADDR_DUNK_TRAFFIC_SUCC) = 1.0f;
                LOG_INFO("[MatchController] Highlight Dunk Boost ENABLED (Maximum Posterizers)");
            } else {
                *reinterpret_cast<float*>(ADDR_DUNK_TRAFFIC_FREQ) = m_SavedDunkFreq;
                *reinterpret_cast<float*>(ADDR_DUNK_TRAFFIC_SUCC) = m_SavedDunkSucc;
                LOG_INFO("[MatchController] Highlight Dunk Boost DISABLED");
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void MatchController::SetCleanDefense(bool enabled) {
        m_CleanDefense = enabled;
        __try {
            if (enabled) {
                m_SavedReachFoul = *reinterpret_cast<const float*>(ADDR_REACHING_FOUL);
                *reinterpret_cast<float*>(ADDR_REACHING_FOUL) = 0.0f;
                LOG_INFO("[MatchController] Clean Defense ENABLED (Reaching Fouls set to 0.00)");
            } else {
                *reinterpret_cast<float*>(ADDR_REACHING_FOUL) = m_SavedReachFoul;
                LOG_INFO("[MatchController] Clean Defense DISABLED");
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void MatchController::SetDisableLateGameFouling(bool disabled) {
        m_DisableLateGameFouling = disabled;
        __try {
            *reinterpret_cast<uint32_t*>(ADDR_DISABLE_LATE_GAME_FOULING_TEAM0) = disabled ? 1 : 0;
            *reinterpret_cast<uint32_t*>(ADDR_DISABLE_LATE_GAME_FOULING_TEAM1) = disabled ? 1 : 0;
            LOG_INFO("[MatchController] Late-Game AI Intentional Fouling: %s", disabled ? "DISABLED" : "NORMAL");
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void MatchController::SetZeroFoulsMode(bool enabled) {
        m_ZeroFoulsMode = enabled;
        __try {
            if (enabled) {
                if (!m_FoulsSaved) {
                    m_SavedChargingFoul  = *reinterpret_cast<const float*>(ADDR_CHARGING_FOUL);
                    m_SavedBlockingFoul  = *reinterpret_cast<const float*>(ADDR_BLOCKING_FOUL);
                    m_SavedReachFoul     = *reinterpret_cast<const float*>(ADDR_REACHING_FOUL);
                    m_SavedShootingFoul  = *reinterpret_cast<const float*>(ADDR_SHOOTING_FOUL);
                    m_SavedLooseBallFoul = *reinterpret_cast<const float*>(ADDR_LOOSE_BALL_FOUL);
                    m_FoulsSaved = true;
                }
                *reinterpret_cast<float*>(ADDR_CHARGING_FOUL)   = 0.0f;
                *reinterpret_cast<float*>(ADDR_BLOCKING_FOUL)   = 0.0f;
                *reinterpret_cast<float*>(ADDR_REACHING_FOUL)   = 0.0f;
                *reinterpret_cast<float*>(ADDR_SHOOTING_FOUL)   = 0.0f;
                *reinterpret_cast<float*>(ADDR_LOOSE_BALL_FOUL) = 0.0f;
                LOG_INFO("[MatchController] Zero Fouls Mode (Immaculate Defense) ENABLED");
            } else {
                if (m_FoulsSaved) {
                    *reinterpret_cast<float*>(ADDR_CHARGING_FOUL)   = m_SavedChargingFoul;
                    *reinterpret_cast<float*>(ADDR_BLOCKING_FOUL)   = m_SavedBlockingFoul;
                    *reinterpret_cast<float*>(ADDR_REACHING_FOUL)   = m_SavedReachFoul;
                    *reinterpret_cast<float*>(ADDR_SHOOTING_FOUL)   = m_SavedShootingFoul;
                    *reinterpret_cast<float*>(ADDR_LOOSE_BALL_FOUL) = m_SavedLooseBallFoul;
                }
                LOG_INFO("[MatchController] Zero Fouls Mode DISABLED");
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
        if (settings.godShooting != m_GodShooting) {
            SetGodShooting(settings.godShooting);
        }
        if (settings.highlightDunkBoost != m_HighlightDunkBoost) {
            SetHighlightDunkBoost(settings.highlightDunkBoost);
        }
        if (settings.cleanDefense != m_CleanDefense) {
            SetCleanDefense(settings.cleanDefense);
        }
        if (settings.disableLateGameFouling != m_DisableLateGameFouling) {
            SetDisableLateGameFouling(settings.disableLateGameFouling);
        }
        if (settings.zeroFoulsMode != m_ZeroFoulsMode) {
            SetZeroFoulsMode(settings.zeroFoulsMode);
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

        if (m_GodShooting) {
            __try {
                *reinterpret_cast<float*>(ADDR_SHOT_INSIDE_USER) = 1.0f;
                *reinterpret_cast<float*>(ADDR_SHOT_CLOSE_USER)  = 1.0f;
                *reinterpret_cast<float*>(ADDR_SHOT_MID_USER)    = 1.0f;
                *reinterpret_cast<float*>(ADDR_SHOT_3PT_USER)    = 1.0f;
                *reinterpret_cast<float*>(ADDR_SHOT_LAYUP_USER)  = 1.0f;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        if (m_HighlightDunkBoost) {
            __try {
                *reinterpret_cast<float*>(ADDR_DUNK_TRAFFIC_FREQ) = 1.0f;
                *reinterpret_cast<float*>(ADDR_DUNK_TRAFFIC_SUCC) = 1.0f;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        if (m_CleanDefense) {
            __try {
                *reinterpret_cast<float*>(ADDR_REACHING_FOUL) = 0.0f;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        if (m_ZeroFoulsMode) {
            __try {
                *reinterpret_cast<float*>(ADDR_CHARGING_FOUL)   = 0.0f;
                *reinterpret_cast<float*>(ADDR_BLOCKING_FOUL)   = 0.0f;
                *reinterpret_cast<float*>(ADDR_REACHING_FOUL)   = 0.0f;
                *reinterpret_cast<float*>(ADDR_SHOOTING_FOUL)   = 0.0f;
                *reinterpret_cast<float*>(ADDR_LOOSE_BALL_FOUL) = 0.0f;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        if (m_DisableLateGameFouling) {
            __try {
                *reinterpret_cast<uint32_t*>(ADDR_DISABLE_LATE_GAME_FOULING_TEAM0) = 1;
                *reinterpret_cast<uint32_t*>(ADDR_DISABLE_LATE_GAME_FOULING_TEAM1) = 1;
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
