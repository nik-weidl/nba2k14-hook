#pragma once
#include <cstdint>

namespace Features {
    class MatchController {
    public:
        static MatchController& Get();

        void Init();
        void Update();

        // Status
        bool IsInMatch();

        // Shot Clock
        float GetShotClock();
        void SetShotClock(float seconds);
        void ResetShotClock24();
        void ResetShotClock14();
        void SetShotClockPressure();
        void SetFreezeShotClock(bool freeze);
        bool IsShotClockFrozen() const { return m_FreezeShotClock; }

        // Game Quarter Clock
        float GetGameClock();
        void SetGameClock(float seconds);
        void AddGameClock(float seconds);
        void InstantEndOfQuarter();
        void SetFreezeGameClock(bool freeze);
        bool IsGameClockFrozen() const { return m_FreezeGameClock; }

        // Stamina & Fatigue
        void SetUnlimitedStamina(bool enabled);
        bool IsUnlimitedStamina() const { return m_UnlimitedStamina; }

        // Shooting & Scoring Assist
        void SetGodShooting(bool enabled);
        bool IsGodShooting() const { return m_GodShooting; }

        // Highlight Contact Dunks & Posterizers
        void SetHighlightDunkBoost(bool enabled);
        bool IsHighlightDunkBoost() const { return m_HighlightDunkBoost; }

        // Clean Defense (No Reach-In Fouls)
        void SetCleanDefense(bool enabled);
        bool IsCleanDefense() const { return m_CleanDefense; }

        // Late Game Intentional Fouling (Stops AI foul spam in clutch)
        void SetDisableLateGameFouling(bool disabled);
        bool IsDisableLateGameFouling() const { return m_DisableLateGameFouling; }

        // Zero Fouls Mode (Immaculate Defense across all foul types)
        void SetZeroFoulsMode(bool enabled);
        bool IsZeroFoulsMode() const { return m_ZeroFoulsMode; }

    private:
        MatchController() = default;

        bool m_GodShooting = false;
        bool m_HighlightDunkBoost = false;
        bool m_CleanDefense = false;
        bool m_DisableLateGameFouling = false;
        bool m_ZeroFoulsMode = false;
        bool m_SlidersSaved = false;
        bool m_FoulsSaved = false;

        float m_SavedInsideShot = 0.5f;
        float m_SavedCloseShot = 0.5f;
        float m_SavedMidShot = 0.5f;
        float m_Saved3ptShot = 0.5f;
        float m_SavedLayup = 0.5f;
        float m_SavedDunkFreq = 0.5f;
        float m_SavedDunkSucc = 0.5f;
        float m_SavedReachFoul = 0.5f;

        float m_SavedChargingFoul = 0.5f;
        float m_SavedBlockingFoul = 0.5f;
        float m_SavedShootingFoul = 0.5f;
        float m_SavedLooseBallFoul = 0.5f;

        bool m_FreezeShotClock = false;
        float m_FrozenShotClockValue = 24.0f;

        bool m_FreezeGameClock = false;
        float m_FrozenGameClockValue = 720.0f;

        bool m_UnlimitedStamina = false;
        float m_SavedUserFatigueRate = 0.5f;
        float m_SavedCpuFatigueRate = 0.5f;
        bool m_FatigueSaved = false;

        // Hotkey states
        bool m_PrevF9Down = false;
        bool m_PrevF10Down = false;
        bool m_PrevF11Down = false;
        bool m_PrevF12Down = false;
    };
}
