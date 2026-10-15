#include "feature_manager.h"
#include "match_controller.h"
#include "../core/logger.h"
#include "../hooks/speed_hook.h"
#include "../hooks/badge_hook.h"
#include <windows.h>
#include <sstream>

namespace Features {
    static bool s_PrevBulletKeyDown = false;
    static bool s_BulletTimeToggleState = false;

    FeatureManager& FeatureManager::Get() {
        static FeatureManager instance;
        return instance;
    }

    void FeatureManager::Init() {
        LOG_INFO("Initializing Feature Manager...");

        // Initialize Game Speed Engine Hook
        Hooks::SpeedHook::Init();

        // Initialize Signature Skills Master Unlocker Hook
        Hooks::BadgeHook::Init();

        // Initialize Match Controller (Clocks & Fatigue)
        Features::MatchController::Get().Init();

        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string dir(exePath);
        size_t lastSlash = dir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            dir = dir.substr(0, lastSlash);
        }

        m_ConfigPath = dir + "\\nba2k14_hook_settings.ini";
        LoadConfig();
    }

    void FeatureManager::LoadConfig() {
        if (m_ConfigPath.empty()) return;

        DWORD attrib = GetFileAttributesA(m_ConfigPath.c_str());
        if (attrib == INVALID_FILE_ATTRIBUTES) {
            LOG_INFO("[FeatureManager] Settings file not found (%s), saving initial defaults.", m_ConfigPath.c_str());
            SaveConfig();
            return;
        }

        m_Settings.showFpsOverlay = (GetPrivateProfileIntA("Overlay", "ShowFpsOverlay", m_Settings.showFpsOverlay ? 1 : 0, m_ConfigPath.c_str()) != 0);
        m_Settings.vfsEnabled = (GetPrivateProfileIntA("VFS", "Enabled", m_Settings.vfsEnabled ? 1 : 0, m_ConfigPath.c_str()) != 0);
        m_Settings.toggleKey = GetPrivateProfileIntA("Hotkeys", "ToggleKey", m_Settings.toggleKey, m_ConfigPath.c_str());

        // Gameplay - Speed & Bullet Time
        m_Settings.speedHackEnabled = (GetPrivateProfileIntA("Gameplay", "SpeedHackEnabled", m_Settings.speedHackEnabled ? 1 : 0, m_ConfigPath.c_str()) != 0);
        char speedBuf[32];
        if (GetPrivateProfileStringA("Gameplay", "GameSpeed", "1.0", speedBuf, sizeof(speedBuf), m_ConfigPath.c_str()) > 0) {
            m_Settings.gameSpeed = static_cast<float>(atof(speedBuf));
            if (m_Settings.gameSpeed <= 0.05f) m_Settings.gameSpeed = 1.0f;
        }

        m_Settings.bulletTimeEnabled = (GetPrivateProfileIntA("Gameplay", "BulletTimeEnabled", m_Settings.bulletTimeEnabled ? 1 : 0, m_ConfigPath.c_str()) != 0);
        if (GetPrivateProfileStringA("Gameplay", "BulletTimeSpeed", "0.25", speedBuf, sizeof(speedBuf), m_ConfigPath.c_str()) > 0) {
            m_Settings.bulletTimeSpeed = static_cast<float>(atof(speedBuf));
            if (m_Settings.bulletTimeSpeed <= 0.0f) m_Settings.bulletTimeSpeed = 0.25f;
        }

        m_Settings.bulletTimeKey = GetPrivateProfileIntA("Gameplay", "BulletTimeKey", m_Settings.bulletTimeKey, m_ConfigPath.c_str());
        m_Settings.bulletTimeHoldMode = (GetPrivateProfileIntA("Gameplay", "BulletTimeHoldMode", m_Settings.bulletTimeHoldMode ? 1 : 0, m_ConfigPath.c_str()) != 0);
        m_Settings.unlimitedStamina = (GetPrivateProfileIntA("Gameplay", "UnlimitedStamina", m_Settings.unlimitedStamina ? 1 : 0, m_ConfigPath.c_str()) != 0);
        m_Settings.godShooting = (GetPrivateProfileIntA("Gameplay", "GodShooting", m_Settings.godShooting ? 1 : 0, m_ConfigPath.c_str()) != 0);
        m_Settings.highlightDunkBoost = (GetPrivateProfileIntA("Gameplay", "HighlightDunkBoost", m_Settings.highlightDunkBoost ? 1 : 0, m_ConfigPath.c_str()) != 0);
        m_Settings.cleanDefense = (GetPrivateProfileIntA("Gameplay", "CleanDefense", m_Settings.cleanDefense ? 1 : 0, m_ConfigPath.c_str()) != 0);
        m_Settings.unlockAllBadges = (GetPrivateProfileIntA("Gameplay", "UnlockAllBadges", m_Settings.unlockAllBadges ? 1 : 0, m_ConfigPath.c_str()) != 0);
        m_Settings.badgesTargetMode = GetPrivateProfileIntA("Gameplay", "BadgesTargetMode", m_Settings.badgesTargetMode, m_ConfigPath.c_str());
        m_Settings.badgesTier = GetPrivateProfileIntA("Gameplay", "BadgesTier", m_Settings.badgesTier, m_ConfigPath.c_str());

        LOG_INFO("[FeatureManager] Loaded settings from %s (Speed: %.2fx, BulletTime: %d)",
            m_ConfigPath.c_str(), m_Settings.gameSpeed, m_Settings.bulletTimeEnabled ? 1 : 0);
    }

    void FeatureManager::SaveConfig() {
        if (m_ConfigPath.empty()) return;

        WritePrivateProfileStringA("Overlay", "ShowFpsOverlay", m_Settings.showFpsOverlay ? "1" : "0", m_ConfigPath.c_str());
        WritePrivateProfileStringA("VFS", "Enabled", m_Settings.vfsEnabled ? "1" : "0", m_ConfigPath.c_str());
        WritePrivateProfileStringA("Hotkeys", "ToggleKey", std::to_string(m_Settings.toggleKey).c_str(), m_ConfigPath.c_str());

        // Gameplay - Speed & Bullet Time
        WritePrivateProfileStringA("Gameplay", "SpeedHackEnabled", m_Settings.speedHackEnabled ? "1" : "0", m_ConfigPath.c_str());
        
        char speedStr[32];
        sprintf_s(speedStr, sizeof(speedStr), "%.2f", m_Settings.gameSpeed);
        WritePrivateProfileStringA("Gameplay", "GameSpeed", speedStr, m_ConfigPath.c_str());

        WritePrivateProfileStringA("Gameplay", "BulletTimeEnabled", m_Settings.bulletTimeEnabled ? "1" : "0", m_ConfigPath.c_str());
        
        char btSpeedStr[32];
        sprintf_s(btSpeedStr, sizeof(btSpeedStr), "%.2f", m_Settings.bulletTimeSpeed);
        WritePrivateProfileStringA("Gameplay", "BulletTimeSpeed", btSpeedStr, m_ConfigPath.c_str());

        WritePrivateProfileStringA("Gameplay", "BulletTimeKey", std::to_string(m_Settings.bulletTimeKey).c_str(), m_ConfigPath.c_str());
        WritePrivateProfileStringA("Gameplay", "BulletTimeHoldMode", m_Settings.bulletTimeHoldMode ? "1" : "0", m_ConfigPath.c_str());
        WritePrivateProfileStringA("Gameplay", "UnlimitedStamina", m_Settings.unlimitedStamina ? "1" : "0", m_ConfigPath.c_str());
        WritePrivateProfileStringA("Gameplay", "GodShooting", m_Settings.godShooting ? "1" : "0", m_ConfigPath.c_str());
        WritePrivateProfileStringA("Gameplay", "HighlightDunkBoost", m_Settings.highlightDunkBoost ? "1" : "0", m_ConfigPath.c_str());
        WritePrivateProfileStringA("Gameplay", "CleanDefense", m_Settings.cleanDefense ? "1" : "0", m_ConfigPath.c_str());
        WritePrivateProfileStringA("Gameplay", "UnlockAllBadges", m_Settings.unlockAllBadges ? "1" : "0", m_ConfigPath.c_str());
        WritePrivateProfileStringA("Gameplay", "BadgesTargetMode", std::to_string(m_Settings.badgesTargetMode).c_str(), m_ConfigPath.c_str());
        WritePrivateProfileStringA("Gameplay", "BadgesTier", std::to_string(m_Settings.badgesTier).c_str(), m_ConfigPath.c_str());

        LOG_INFO("[FeatureManager] Saved settings to %s", m_ConfigPath.c_str());
    }

    void FeatureManager::Update() {
        // Handle Bullet-Time Hotkey
        bool bulletTimeActive = false;

        if (m_Settings.bulletTimeEnabled && !m_Settings.showMenu) {
            bool isKeyDown = (GetAsyncKeyState(m_Settings.bulletTimeKey) & 0x8000) != 0;

            if (m_Settings.bulletTimeHoldMode) {
                bulletTimeActive = isKeyDown;
            } else {
                // Toggle mode: toggle on key press down
                if (isKeyDown && !s_PrevBulletKeyDown) {
                    s_BulletTimeToggleState = !s_BulletTimeToggleState;
                }
                bulletTimeActive = s_BulletTimeToggleState;
            }
            s_PrevBulletKeyDown = isKeyDown;
        } else {
            if (m_Settings.bulletTimeHoldMode) {
                bulletTimeActive = false;
            } else {
                bulletTimeActive = s_BulletTimeToggleState;
            }
        }

        // Calculate active effective speed
        float targetSpeed = m_Settings.gameSpeed;
        if (m_Settings.freezeTime) {
            targetSpeed = 0.0f;
        } else if (bulletTimeActive) {
            targetSpeed = m_Settings.bulletTimeSpeed;
        }

        bool speedActive = m_Settings.speedHackEnabled || bulletTimeActive || m_Settings.freezeTime;
        Hooks::SpeedHook::SetEnabled(speedActive);
        Hooks::SpeedHook::SetSpeed(m_Settings.gameSpeed);
        Hooks::SpeedHook::SetEffectiveSpeed(targetSpeed);
        Hooks::SpeedHook::SetBulletTimeActive(bulletTimeActive);

        // Update Signature Skills Master Unlocker Hook
        Hooks::BadgeHook::SetEnabled(m_Settings.unlockAllBadges);
        Hooks::BadgeHook::SetTargetMode(m_Settings.badgesTargetMode);
        Hooks::BadgeHook::SetTier(m_Settings.badgesTier);

        // Update Match Controller (Clock freezing & Unlimited Stamina)
        Features::MatchController::Get().Update();
    }
}

