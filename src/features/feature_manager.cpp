#include "feature_manager.h"
#include "../core/logger.h"
#include <windows.h>

namespace Features {
    FeatureManager& FeatureManager::Get() {
        static FeatureManager instance;
        return instance;
    }

    void FeatureManager::Init() {
        LOG_INFO("Initializing Feature Manager...");

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

        LOG_INFO("[FeatureManager] Loaded settings from %s (showFpsOverlay: %d)",
            m_ConfigPath.c_str(), m_Settings.showFpsOverlay ? 1 : 0);
    }

    void FeatureManager::SaveConfig() {
        if (m_ConfigPath.empty()) return;

        WritePrivateProfileStringA("Overlay", "ShowFpsOverlay", m_Settings.showFpsOverlay ? "1" : "0", m_ConfigPath.c_str());
        WritePrivateProfileStringA("VFS", "Enabled", m_Settings.vfsEnabled ? "1" : "0", m_ConfigPath.c_str());
        WritePrivateProfileStringA("Hotkeys", "ToggleKey", std::to_string(m_Settings.toggleKey).c_str(), m_ConfigPath.c_str());

        LOG_INFO("[FeatureManager] Saved settings to %s (showFpsOverlay: %d)",
            m_ConfigPath.c_str(), m_Settings.showFpsOverlay ? 1 : 0);
    }

    void FeatureManager::Update() {
        // Run per-frame game modifications when active
        if (m_Settings.speedHackEnabled) {
            // Can modify game delta-time or timer scale
        }
    }
}
