#pragma once
#include <string>

namespace Features {
    struct Settings {
        bool showMenu = false;
        int toggleKey = 0x77; // VK_F8 (0x77) or VK_INSERT (0x2D)
        
        // Gameplay settings
        bool speedHackEnabled = false;
        float gameSpeed = 1.0f;
        
        bool unlimitedStamina = false;
        bool shotMeterAlwaysGreen = false;
        bool unlockAllAnimations = false;
        bool freeCameraEnabled = false;
        float cameraFov = 60.0f;
        
        // Internal Debug Toggles
        bool showFpsOverlay = true;
        bool enableEngineDebugText = false;
        
        // Virtual File System (Mods)
        bool vfsEnabled = true;
    };

    class FeatureManager {
    public:
        static FeatureManager& Get();

        void Init();
        void Update();
        void LoadConfig();
        void SaveConfig();
        Settings& GetSettings() { return m_Settings; }

    private:
        FeatureManager() = default;
        Settings m_Settings;
        std::string m_ConfigPath;
    };
}
