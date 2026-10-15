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

    private:
        MatchController() = default;

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
