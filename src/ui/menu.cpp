#include "menu.h"
#include "../features/feature_manager.h"
#include "../features/match_controller.h"
#include "../features/vfs.h"
#include "../hooks/speed_hook.h"
#include "../core/logger.h"
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <string>
#include <algorithm>
#include <chrono>

namespace UI {
    bool Menu::s_Initialized = false;
    HWND Menu::s_Hwnd = nullptr;
    static std::chrono::steady_clock::time_point s_BootTime;
    static bool s_BootTimeRecorded = false;
    static char s_ModSearchFilter[128] = "";
    static int s_TargetingModIndex = -1;
    static int s_SelectedTeamIdx = 0;
    static int s_SelectedSlotIdx = 0;
    static char s_CustomTargetInput[64] = "";
    static bool s_OpenTargetModal = false;

    static std::string ToLower(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    void Menu::ApplyTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 8.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 5.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 9.0f;
        style.GrabRounding = 5.0f;
        style.TabRounding = 6.0f;

        style.WindowPadding = ImVec2(14.0f, 14.0f);
        style.FramePadding = ImVec2(8.0f, 5.0f);
        style.ItemSpacing = ImVec2(10.0f, 8.0f);

        ImVec4* colors = style.Colors;
        // Premium Dark Slate Theme with Vibrant Electric Blue Accent
        colors[ImGuiCol_Text]                  = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.53f, 0.58f, 1.00f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.09f, 0.10f, 0.13f, 0.96f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.12f, 0.14f, 0.18f, 0.70f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.10f, 0.12f, 0.15f, 0.98f);
        colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.23f, 0.29f, 0.60f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.15f, 0.17f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.20f, 0.24f, 0.31f, 1.00f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.24f, 0.28f, 0.36f, 1.00f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.07f, 0.08f, 0.11f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.07f, 0.08f, 0.11f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.07f, 0.08f, 0.11f, 0.75f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.08f, 0.09f, 0.11f, 0.60f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.22f, 0.25f, 0.32f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.28f, 0.32f, 0.40f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.35f, 0.40f, 0.50f, 1.00f);
        colors[ImGuiCol_CheckMark]             = ImVec4(0.18f, 0.65f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab]            = ImVec4(0.18f, 0.65f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.35f, 0.75f, 1.00f, 1.00f);
        colors[ImGuiCol_Button]                = ImVec4(0.16f, 0.45f, 0.85f, 0.85f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.20f, 0.55f, 0.95f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.14f, 0.38f, 0.75f, 1.00f);
        colors[ImGuiCol_Header]                = ImVec4(0.16f, 0.45f, 0.85f, 0.45f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.20f, 0.55f, 0.95f, 0.70f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.24f, 0.60f, 1.00f, 0.90f);
        colors[ImGuiCol_Separator]             = ImVec4(0.22f, 0.25f, 0.32f, 0.60f);
        colors[ImGuiCol_Tab]                   = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.20f, 0.50f, 0.90f, 0.80f);
        colors[ImGuiCol_TabActive]             = ImVec4(0.16f, 0.45f, 0.85f, 1.00f);
        colors[ImGuiCol_TabUnfocused]          = ImVec4(0.10f, 0.11f, 0.15f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.13f, 0.28f, 0.50f, 1.00f);
    }

    void Menu::Init(IDirect3DDevice9* pDevice, HWND hWnd) {
        if (s_Initialized) return;

        s_Hwnd = hWnd;
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui_ImplWin32_Init(hWnd);
        ImGui_ImplDX9_Init(pDevice);

        ApplyTheme();

        s_BootTime = std::chrono::steady_clock::now();
        s_BootTimeRecorded = true;
        s_Initialized = true;
        LOG_INFO("ImGui Menu successfully initialized.");
    }

    void Menu::Shutdown() {
        if (!s_Initialized) return;
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        s_Initialized = false;
        LOG_INFO("ImGui Menu shutdown.");
    }

    void Menu::Toggle() {
        auto& settings = Features::FeatureManager::Get().GetSettings();
        settings.showMenu = !settings.showMenu;
        LOG_INFO("Menu toggled: %s", settings.showMenu ? "VISIBLE" : "HIDDEN");
    }

    bool Menu::IsVisible() {
        return Features::FeatureManager::Get().GetSettings().showMenu;
    }

    void Menu::Render() {
        auto& settings = Features::FeatureManager::Get().GetSettings();
        auto& vfs = Features::VFS::Get();

        if (!s_BootTimeRecorded) {
            s_BootTime = std::chrono::steady_clock::now();
            s_BootTimeRecorded = true;
        }

        // 1. Watermark / Small HUD
        // If showFpsOverlay is checked: permanently visible.
        // If showFpsOverlay is unchecked: visible for 2.0s on game boot, then smoothly fades out over 0.8s.
        float watermarkAlpha = 1.0f;
        bool shouldRenderWatermark = settings.showFpsOverlay;

        if (!shouldRenderWatermark) {
            auto now = std::chrono::steady_clock::now();
            float elapsedSec = std::chrono::duration<float>(now - s_BootTime).count();
            const float holdSec = 2.0f;
            const float fadeSec = 0.8f;

            if (elapsedSec < holdSec) {
                shouldRenderWatermark = true;
                watermarkAlpha = 1.0f;
            } else if (elapsedSec < holdSec + fadeSec) {
                shouldRenderWatermark = true;
                watermarkAlpha = 1.0f - ((elapsedSec - holdSec) / fadeSec);
                if (watermarkAlpha < 0.0f) watermarkAlpha = 0.0f;
                if (watermarkAlpha > 1.0f) watermarkAlpha = 1.0f;
            } else {
                shouldRenderWatermark = false;
            }
        }

        if (shouldRenderWatermark && watermarkAlpha > 0.001f) {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.60f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, watermarkAlpha);

            ImGuiWindowFlags hudFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                        ImGuiWindowFlags_NoNav;

            if (ImGui::Begin("##Watermark", nullptr, hudFlags)) {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "NBA 2K14 Hook");
                ImGui::SameLine();
                ImGui::Text("| %.1f FPS", io.Framerate);
                ImGui::SameLine();
                if (vfs.GetModCount() > 0) {
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "| %zu/%zu Mods Active", vfs.GetEnabledModCount(), vfs.GetModCount());
                    ImGui::SameLine();
                }

                if (Hooks::SpeedHook::IsEnabled()) {
                    float effSpeed = Hooks::SpeedHook::GetEffectiveSpeed();
                    if (Hooks::SpeedHook::IsBulletTimeActive()) {
                        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.2f, 1.0f), "| Slomo: %.2fx", effSpeed);
                        ImGui::SameLine();
                    } else if (effSpeed == 0.0f) {
                        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "| Time Frozen");
                        ImGui::SameLine();
                    } else if (effSpeed != 1.0f) {
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "| Speed: %.2fx", effSpeed);
                        ImGui::SameLine();
                    }
                }

                ImGui::TextDisabled("| [F8] Menu");
                ImGui::End();
            }

            ImGui::PopStyleVar();
        }

        // 2. Main Menu Window (Toggled via F8 or Insert)
        if (!settings.showMenu) return;

        ImGui::SetNextWindowSize(ImVec2(760, 540), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("NBA 2K14 Feature Hook", &settings.showMenu, ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::BeginTabBar("MainTabs")) {
                
                // Tab: Dashboard
                if (ImGui::BeginTabItem("Dashboard")) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Native Mod & Feature Framework");
                    ImGui::TextWrapped("Welcome to the NBA 2K14 Native Hook. Use the tabs above to configure real-time gameplay features, cameras, visual enhancements, and manage active mods.");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Columns(2, "dash_cols", false);
                    ImGui::Text("Hook Status:");
                    ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "ACTIVE (DirectX 9Ex Hooked)");
                    ImGui::NextColumn();

                    ImGui::Text("Executable:");
                    ImGui::NextColumn();
                    ImGui::Text("nba2k14.exe (Base: 0x00400000)");
                    ImGui::NextColumn();

                    ImGui::Text("Virtual File System (VFS):");
                    ImGui::NextColumn();
                    if (vfs.IsEnabled()) {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "ONLINE (%zu of %zu active, %zu redirected)",
                            vfs.GetEnabledModCount(), vfs.GetModCount(), vfs.GetTotalRedirections());
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "DISABLED");
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Toggle Hotkey:");
                    ImGui::NextColumn();
                    ImGui::Text("F8 / Insert");
                    ImGui::Columns(1);

                    ImGui::Spacing();
                    ImGui::Separator();
                    if (ImGui::Checkbox("Show FPS & Status Overlay", &settings.showFpsOverlay)) {
                        Features::FeatureManager::Get().SaveConfig();
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("When unchecked, the overlay only appears for 2 seconds on game startup then fades away.");
                    }

                    ImGui::EndTabItem();
                }

                // Tab: Gameplay
                if (ImGui::BeginTabItem("Gameplay")) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Game Speed & Physics Clock Engine");
                    ImGui::TextWrapped("Control the real-time simulation tick rate of player animations, ball physics, and game clocks without desyncing audio or rendering.");
                    ImGui::Separator();
                    ImGui::Spacing();

                    // Speed Status Telemetry
                    float effSpeed = Hooks::SpeedHook::GetEffectiveSpeed();
                    bool isSpeedActive = Hooks::SpeedHook::IsEnabled();
                    bool isBulletActive = Hooks::SpeedHook::IsBulletTimeActive();

                    ImGui::Text("Active Simulation Rate:");
                    ImGui::SameLine();
                    if (!isSpeedActive || effSpeed == 1.0f) {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "1.00x [NORMAL TIME]");
                    } else if (isBulletActive) {
                        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.2f, 1.0f), "%.2fx [BULLET-TIME ACTIVE]", effSpeed);
                    } else if (settings.freezeTime) {
                        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "0.00x [TIME FROZEN]");
                    } else if (effSpeed < 1.0f) {
                        ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.2f, 1.0f), "%.2fx [SLOW MOTION]", effSpeed);
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%.2fx [TURBO FAST-FORWARD]", effSpeed);
                    }

                    ImGui::Spacing();

                    // Game Speed Multiplier
                    if (ImGui::Checkbox("Enable Custom Game Speed Multiplier", &settings.speedHackEnabled)) {
                        Features::FeatureManager::Get().SaveConfig();
                    }

                    if (settings.speedHackEnabled) {
                        ImGui::Indent(15.0f);
                        if (ImGui::SliderFloat("Speed Scale", &settings.gameSpeed, 0.10f, 3.00f, "%.2fx")) {
                            Features::FeatureManager::Get().SaveConfig();
                        }

                        // Presets
                        ImGui::TextDisabled("Quick Presets:");
                        ImGui::SameLine();
                        if (ImGui::SmallButton("0.25x (Super Slomo)")) { settings.gameSpeed = 0.25f; Features::FeatureManager::Get().SaveConfig(); }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("0.50x (Half)")) { settings.gameSpeed = 0.50f; Features::FeatureManager::Get().SaveConfig(); }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("1.00x (Normal)")) { settings.gameSpeed = 1.00f; Features::FeatureManager::Get().SaveConfig(); }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("1.50x (Fast)")) { settings.gameSpeed = 1.50f; Features::FeatureManager::Get().SaveConfig(); }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("2.00x (Turbo)")) { settings.gameSpeed = 2.00f; Features::FeatureManager::Get().SaveConfig(); }

                        if (ImGui::Checkbox("Freeze Time (0.00x - Pause Gameplay Only)", &settings.freezeTime)) {
                            // Keeps menus and UI fully responsive while pausing gameplay
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Completely pauses gameplay motion, animations, and game clock while keeping the camera and overlay fully interactive.");
                        }

                        ImGui::Unindent(15.0f);
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // Cinematic Bullet Time
                    ImGui::TextColored(ImVec4(0.95f, 0.6f, 0.2f, 1.0f), "Cinematic Bullet-Time (In-Game Slomo Hotkey)");
                    ImGui::TextWrapped("Hold or toggle a hotkey during live gameplay to instantly slow down time on dunks, fast breaks, blocks, and buzzer-beaters.");

                    if (ImGui::Checkbox("Enable Bullet-Time Hotkey", &settings.bulletTimeEnabled)) {
                        Features::FeatureManager::Get().SaveConfig();
                    }

                    if (settings.bulletTimeEnabled) {
                        ImGui::Indent(15.0f);
                        if (ImGui::SliderFloat("Bullet-Time Speed", &settings.bulletTimeSpeed, 0.10f, 0.75f, "%.2fx")) {
                            Features::FeatureManager::Get().SaveConfig();
                        }

                        // Key selector
                        static const char* s_KeyNames[] = {
                            "Left Alt (VK_MENU)",
                            "F6 Key",
                            "F7 Key",
                            "Middle Mouse Button",
                            "Mouse Button 4 (Thumb Back)",
                            "Mouse Button 5 (Thumb Forward)",
                            "Left Shift",
                            "Caps Lock",
                            "Tilde (~)"
                        };
                        static const int s_KeyVKs[] = {
                            0x12, 0x75, 0x76, 0x04, 0x05, 0x06, 0xA0, 0x14, 0xC0
                        };

                        int selectedKeyIdx = 0;
                        for (int i = 0; i < IM_ARRAYSIZE(s_KeyVKs); i++) {
                            if (s_KeyVKs[i] == settings.bulletTimeKey) {
                                selectedKeyIdx = i;
                                break;
                            }
                        }

                        if (ImGui::Combo("Bullet-Time Hotkey", &selectedKeyIdx, s_KeyNames, IM_ARRAYSIZE(s_KeyNames))) {
                            settings.bulletTimeKey = s_KeyVKs[selectedKeyIdx];
                            Features::FeatureManager::Get().SaveConfig();
                        }

                        // Mode: Hold vs Toggle
                        ImGui::Text("Activation Mode:");
                        ImGui::SameLine();
                        if (ImGui::RadioButton("Hold Key", settings.bulletTimeHoldMode == true)) {
                            settings.bulletTimeHoldMode = true;
                            Features::FeatureManager::Get().SaveConfig();
                        }
                        ImGui::SameLine();
                        if (ImGui::RadioButton("Press to Toggle", settings.bulletTimeHoldMode == false)) {
                            settings.bulletTimeHoldMode = false;
                            Features::FeatureManager::Get().SaveConfig();
                        }

                        ImGui::Unindent(15.0f);
                    }

                    // Match Clocks & Rules
                    auto& match = Features::MatchController::Get();

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Match Clocks & Shot Clock Controller");
                    ImGui::TextWrapped("Directly manipulate live quarter game clocks, 24-second shot clocks, and player energy levels without menu delays.");
                    ImGui::Spacing();

                    if (match.IsInMatch()) {
                        float gClock = match.GetGameClock();
                        float sClock = match.GetShotClock();
                        int minutes = static_cast<int>(gClock) / 60;
                        int seconds = static_cast<int>(gClock) % 60;
                        int tenths = static_cast<int>((gClock - static_cast<float>(static_cast<int>(gClock))) * 10.0f);

                        ImGui::Text("Quarter Clock:");
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "%02d:%02d.%d", minutes, seconds, tenths);

                        ImGui::SameLine(250.0f);
                        ImGui::Text("Shot Clock:");
                        ImGui::SameLine();
                        
                        ImVec4 scColor = (sClock <= 5.0f) ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) :
                                         (sClock <= 10.0f) ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :
                                         ImVec4(0.2f, 1.0f, 0.4f, 1.0f);
                        ImGui::TextColored(scColor, "%.1fs", sClock);

                        // Shot Clock progress bar
                        float scFrac = (std::min)(1.0f, (std::max)(0.0f, sClock / 24.0f));
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, scColor);
                        char scBuf[32];
                        sprintf_s(scBuf, sizeof(scBuf), "%.1f / 24.0s", sClock);
                        ImGui::ProgressBar(scFrac, ImVec2(-1.0f, 14.0f), scBuf);
                        ImGui::PopStyleColor();
                    } else {
                        ImGui::TextDisabled("Match Status: [Waiting for Match to Start - Addresses Ready]");
                    }

                    ImGui::Spacing();

                    // Shot Clock Controls
                    bool freezeSC = match.IsShotClockFrozen();
                    if (ImGui::Checkbox("Freeze Shot Clock (Infinite Possession) [F11]", &freezeSC)) {
                        match.SetFreezeShotClock(freezeSC);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Locks the 24-second shot clock so it never ticks down. Press F11 in-game to toggle.");
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Reset 24s [F9]")) {
                        match.ResetShotClock24();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset 14s [F10]")) {
                        match.ResetShotClock14();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Buzzer Beater (2.0s)")) {
                        match.SetShotClockPressure();
                    }

                    // Quarter Clock Controls
                    bool freezeGC = match.IsGameClockFrozen();
                    if (ImGui::Checkbox("Freeze Quarter Clock", &freezeGC)) {
                        match.SetFreezeGameClock(freezeGC);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("+1 Min")) {
                        match.AddGameClock(60.0f);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("-1 Min")) {
                        match.AddGameClock(-60.0f);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Clutch 2:00")) {
                        match.SetGameClock(120.0f);
                    }

                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.20f, 0.20f, 0.85f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.25f, 0.25f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
                    if (ImGui::Button("End Quarter [F12]")) {
                        match.InstantEndOfQuarter();
                    }
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Instantly ends the current quarter and triggers the buzzer / post-quarter presentation.");
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // Player Stamina & Energy Section
                    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Player Energy & Fatigue Mechanics");

                    if (ImGui::Checkbox("Unlimited Player Stamina (Zero Fatigue Lock)", &settings.unlimitedStamina)) {
                        match.SetUnlimitedStamina(settings.unlimitedStamina);
                        Features::FeatureManager::Get().SaveConfig();
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Locks the fatigue rate slider in memory to 0.00. Players never exhaust turbo sprint, never suffer 4th quarter rating drops, and never show the Gatorade icon.");
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // Upcoming Gameplay Roadmap features
                    ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.85f, 1.0f), "Upcoming Mechanics (In Pipeline)");
                    ImGui::Checkbox("Perfect Shot Timing Assist (Always Green)", &settings.shotMeterAlwaysGreen);
                    ImGui::Checkbox("Unlock All Signature Moves / Animations", &settings.unlockAllAnimations);

                    ImGui::EndTabItem();
                }

                // Tab: Camera & Visuals
                if (ImGui::BeginTabItem("Camera & Visuals")) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Camera Controls & Visuals");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Checkbox("Enable Free Camera Mode", &settings.freeCameraEnabled);
                    ImGui::SliderFloat("Field of View (FOV)", &settings.cameraFov, 30.0f, 120.0f, "%.1f deg");
                    if (ImGui::Button("Reset Camera FOV (60)")) {
                        settings.cameraFov = 60.0f;
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Checkbox("Enable Engine Debug Visuals (VCFEATURECODE)", &settings.enableEngineDebugText);

                    ImGui::EndTabItem();
                }

                // Tab: Mods (Virtual File System)
                if (ImGui::BeginTabItem("Mods (VFS)")) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Virtual File System (VFS) Mod Loader");
                    ImGui::TextWrapped("Drop loose .iff, .bin, or audio files into the 'mods/' folder. You can enable or disable individual mods below—unchecked mods will automatically fallback to the vanilla game files.");
                    ImGui::Separator();
                    ImGui::Spacing();

                    bool vfsEnabled = vfs.IsEnabled();
                    if (ImGui::Checkbox("Master VFS Redirection", &vfsEnabled)) {
                        vfs.SetEnabled(vfsEnabled);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Open Mods Folder")) {
                        vfs.OpenFolderInExplorer();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Rescan Folder")) {
                        vfs.Rescan();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Enable All")) {
                        vfs.EnableAllMods();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Disable All")) {
                        vfs.DisableAllMods();
                    }

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Active: %zu of %zu enabled  |  Total Redirections: %zu",
                        vfs.GetEnabledModCount(), vfs.GetModCount(), vfs.GetTotalRedirections());
                    ImGui::TextDisabled("Location: %s", vfs.GetModsDirectory().c_str());
                    ImGui::Spacing();

                    auto& modList = vfs.GetLoadedMods();
                    if (modList.empty()) {
                        ImGui::BeginChild("EmptyModsBox", ImVec2(0, 150), true);
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "No loose mod files found in /mods/ yet!");
                        ImGui::Spacing();
                        ImGui::BulletText("Click 'Open Mods Folder' above.");
                        ImGui::BulletText("Drop any modded files (e.g. cyberfaces, jerseys, courts) inside.");
                        ImGui::BulletText("Click 'Rescan Folder' to register them immediately.");
                        ImGui::EndChild();
                    } else {
                        static int s_ModViewMode = 0; // 0 = Grouped by Function & Teams, 1 = Flat Table List
                        
                        ImGui::Text("View Mode:");
                        ImGui::SameLine();
                        ImGui::RadioButton("Grouped by Function & Teams", &s_ModViewMode, 0);
                        ImGui::SameLine();
                        ImGui::RadioButton("Flat File List", &s_ModViewMode, 1);
                        ImGui::Spacing();

                        ImGui::InputTextWithHint("##ModSearch", "Filter by filename, team, player, or category...", s_ModSearchFilter, sizeof(s_ModSearchFilter));
                        ImGui::Spacing();

                        std::string searchLower = s_ModSearchFilter;
                        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

                        if (s_ModViewMode == 0) {
                            // --- GROUPED VIEW (Categories & Subgroups) ---
                            auto categoryGroups = vfs.GetCategoryGroups();

                            ImGui::BeginChild("GroupedModsScroll", ImVec2(0, 270), false, ImGuiWindowFlags_HorizontalScrollbar);

                            for (const auto& group : categoryGroups) {
                                // Count active items in category
                                size_t catTotal = 0;
                                size_t catActive = 0;
                                bool anyMatchesSearch = false;

                                for (const auto& sgPair : group.subGroupIndices) {
                                    for (size_t idx : sgPair.second) {
                                        catTotal++;
                                        if (modList[idx].enabled) catActive++;

                                        if (searchLower.empty()) {
                                            anyMatchesSearch = true;
                                        } else {
                                            std::string fnL = ToLower(modList[idx].filename);
                                            std::string sgL = ToLower(modList[idx].subGroupName);
                                            std::string atL = ToLower(modList[idx].assetTypeName);
                                            std::string catL = ToLower(group.name);
                                            if (fnL.find(searchLower) != std::string::npos ||
                                                sgL.find(searchLower) != std::string::npos ||
                                                atL.find(searchLower) != std::string::npos ||
                                                catL.find(searchLower) != std::string::npos) {
                                                anyMatchesSearch = true;
                                            }
                                        }
                                    }
                                }

                                if (!anyMatchesSearch) continue;

                                ImGui::PushID((int)group.category);

                                std::string headerTitle = group.name + " (" + std::to_string(catActive) + " of " + std::to_string(catTotal) + " active)";
                                if (ImGui::CollapsingHeader(headerTitle.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                                    
                                    // Category Quick Controls
                                    ImGui::SameLine(ImGui::GetWindowWidth() - 200.0f);
                                    if (ImGui::SmallButton("Enable Group")) {
                                        vfs.SetCategoryEnabled(group.category, true);
                                    }
                                    ImGui::SameLine();
                                    if (ImGui::SmallButton("Disable Group")) {
                                        vfs.SetCategoryEnabled(group.category, false);
                                    }

                                    if (group.category == Features::ModCategory::Uncategorized) {
                                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                                            "Notice: These mod files have custom names and do not currently replace a recognized game file slot.");
                                        ImGui::TextDisabled("In the next step, you will be able to choose a target game file slot for each custom mod.");
                                        ImGui::Spacing();
                                    }

                                    // Render Subgroups
                                    for (const auto& subName : group.subGroupNames) {
                                        auto it = group.subGroupIndices.find(subName);
                                        if (it == group.subGroupIndices.end()) continue;

                                        const auto& indices = it->second;

                                        // Search filter check for subgroup
                                        bool subMatches = false;
                                        size_t subActive = 0;
                                        for (size_t idx : indices) {
                                            if (modList[idx].enabled) subActive++;
                                            if (searchLower.empty()) {
                                                subMatches = true;
                                            } else {
                                                std::string fnL = ToLower(modList[idx].filename);
                                                std::string sgL = ToLower(subName);
                                                std::string atL = ToLower(modList[idx].assetTypeName);
                                                if (fnL.find(searchLower) != std::string::npos ||
                                                    sgL.find(searchLower) != std::string::npos ||
                                                    atL.find(searchLower) != std::string::npos) {
                                                    subMatches = true;
                                                }
                                            }
                                        }

                                        if (!subMatches) continue;

                                        ImGui::PushID(subName.c_str());

                                        // Subgroup Checkbox & Title
                                        bool isSubActive = vfs.IsSubGroupEnabled(group.category, subName);
                                        if (ImGui::Checkbox("##sub_active", &isSubActive)) {
                                            vfs.SetSubGroupEnabled(group.category, subName, isSubActive);
                                        }
                                        ImGui::SameLine();

                                        std::string subTitle = subName + " (" + std::to_string(subActive) + " / " + std::to_string(indices.size()) + " active)";
                                        if (ImGui::TreeNodeEx(subName.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "%s", subTitle.c_str())) {
                                            
                                            // Subgroup Table
                                            if (ImGui::BeginTable("SubTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                                                ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                                                ImGui::TableSetupColumn("File Name", ImGuiTableColumnFlags_WidthStretch);
                                                ImGui::TableSetupColumn("Asset Type", ImGuiTableColumnFlags_WidthFixed, 130.0f);
                                                ImGui::TableSetupColumn("Target Slot", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                                                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                                                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 130.0f);
                                                ImGui::TableHeadersRow();

                                                for (size_t modIdx : indices) {
                                                    auto& mod = modList[modIdx];

                                                    ImGui::TableNextRow();

                                                    // Column 0: Checkbox
                                                    ImGui::TableSetColumnIndex(0);
                                                    ImGui::PushID((int)modIdx);
                                                    bool isModOn = mod.enabled;
                                                    if (ImGui::Checkbox("##file_on", &isModOn)) {
                                                        vfs.SetModEnabled(modIdx, isModOn);
                                                    }
                                                    ImGui::PopID();

                                                    // Column 1: File Name
                                                    ImGui::TableSetColumnIndex(1);
                                                    if (mod.enabled) {
                                                        ImGui::Text("%s", mod.filename.c_str());
                                                    } else {
                                                        ImGui::TextDisabled("%s", mod.filename.c_str());
                                                    }

                                                    // Column 2: Asset Type
                                                    ImGui::TableSetColumnIndex(2);
                                                    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%s", mod.assetTypeName.c_str());

                                                    // Column 3: Target Slot
                                                    ImGui::TableSetColumnIndex(3);
                                                    if (!mod.targetFile.empty()) {
                                                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "-> %s", mod.targetFile.c_str());
                                                    } else {
                                                        ImGui::TextDisabled("Default");
                                                    }

                                                    // Column 4: File Size
                                                    ImGui::TableSetColumnIndex(4);
                                                    if (mod.fileSize >= 1024 * 1024) {
                                                        ImGui::Text("%.2f MB", (float)mod.fileSize / (1024.0f * 1024.0f));
                                                    } else {
                                                        ImGui::Text("%.1f KB", (float)mod.fileSize / 1024.0f);
                                                    }

                                                    // Column 5: Actions
                                                    ImGui::TableSetColumnIndex(5);
                                                    ImGui::PushID((int)(20000 + modIdx));
                                                    if (!mod.targetFile.empty()) {
                                                        if (ImGui::SmallButton("Change")) {
                                                            s_TargetingModIndex = (int)modIdx;
                                                            s_OpenTargetModal = true;
                                                        }
                                                        ImGui::SameLine();
                                                        if (ImGui::SmallButton("Clear")) {
                                                            vfs.ClearModTarget(modIdx);
                                                        }
                                                    } else {
                                                        if (ImGui::SmallButton("Target...")) {
                                                            s_TargetingModIndex = (int)modIdx;
                                                            s_OpenTargetModal = true;
                                                        }
                                                    }
                                                    ImGui::PopID();
                                                }
                                                ImGui::EndTable();
                                            }

                                            ImGui::TreePop();
                                        }

                                        ImGui::PopID();
                                        ImGui::Spacing();
                                    }
                                }

                                ImGui::PopID();
                                ImGui::Spacing();
                            }

                            ImGui::EndChild();
                        } else {
                            // --- FLAT TABLE VIEW ---
                            if (ImGui::BeginTable("ModsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 270))) {
                                ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                                ImGui::TableSetupColumn("File Name / Relative Path", ImGuiTableColumnFlags_WidthStretch);
                                ImGui::TableSetupColumn("Category / Subgroup", ImGuiTableColumnFlags_WidthFixed, 160.0f);
                                ImGui::TableSetupColumn("Target Slot", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                                ImGui::TableHeadersRow();

                                for (size_t i = 0; i < modList.size(); i++) {
                                    auto& mod = modList[i];

                                    std::string fnLower = mod.filename;
                                    std::transform(fnLower.begin(), fnLower.end(), fnLower.begin(), ::tolower);
                                    std::string relLower = mod.relativePath;
                                    std::transform(relLower.begin(), relLower.end(), relLower.begin(), ::tolower);
                                    std::string sgLower = mod.subGroupName;
                                    std::transform(sgLower.begin(), sgLower.end(), sgLower.begin(), ::tolower);

                                    if (!searchLower.empty() && fnLower.find(searchLower) == std::string::npos &&
                                        relLower.find(searchLower) == std::string::npos &&
                                        sgLower.find(searchLower) == std::string::npos) {
                                        continue;
                                    }

                                    ImGui::TableNextRow();

                                    // Column 0: Checkbox
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::PushID((int)i);
                                    bool isEnabled = mod.enabled;
                                    if (ImGui::Checkbox("##enabled", &isEnabled)) {
                                        vfs.SetModEnabled(i, isEnabled);
                                    }
                                    ImGui::PopID();

                                    // Column 1: Filename / Path
                                    ImGui::TableSetColumnIndex(1);
                                    if (mod.enabled) {
                                        ImGui::Text("%s", mod.relativePath.c_str());
                                    } else {
                                        ImGui::TextDisabled("%s", mod.relativePath.c_str());
                                    }

                                    // Column 2: Category & Subgroup
                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%s", mod.subGroupName.c_str());

                                    // Column 3: Target Slot
                                    ImGui::TableSetColumnIndex(3);
                                    if (!mod.targetFile.empty()) {
                                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "-> %s", mod.targetFile.c_str());
                                    } else {
                                        ImGui::TextDisabled("Default");
                                    }

                                    // Column 4: File Size
                                    ImGui::TableSetColumnIndex(4);
                                    if (mod.fileSize >= 1024 * 1024) {
                                        ImGui::Text("%.2f MB", (float)mod.fileSize / (1024.0f * 1024.0f));
                                    } else {
                                        ImGui::Text("%.1f KB", (float)mod.fileSize / 1024.0f);
                                    }

                                    // Column 5: Actions
                                    ImGui::TableSetColumnIndex(5);
                                    ImGui::PushID((int)(30000 + i));
                                    if (!mod.targetFile.empty()) {
                                        if (ImGui::SmallButton("Change")) {
                                            s_TargetingModIndex = (int)i;
                                            s_OpenTargetModal = true;
                                        }
                                        ImGui::SameLine();
                                        if (ImGui::SmallButton("Clear")) {
                                            vfs.ClearModTarget(i);
                                        }
                                    } else {
                                        if (ImGui::SmallButton("Target...")) {
                                            s_TargetingModIndex = (int)i;
                                            s_OpenTargetModal = true;
                                        }
                                    }
                                    ImGui::PopID();
                                }
                                ImGui::EndTable();
                            }
                        }

                        // --- INTERACTIVE TARGET CHOOSER MODAL ---
                        if (s_OpenTargetModal) {
                            ImGui::OpenPopup("Assign Game Slot");
                            s_OpenTargetModal = false;
                        }

                        if (ImGui::BeginPopupModal("Assign Game Slot", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                            if (s_TargetingModIndex >= 0 && s_TargetingModIndex < (int)modList.size()) {
                                auto& targetMod = modList[s_TargetingModIndex];

                                ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Mod File: %s", targetMod.filename.c_str());
                                if (!targetMod.targetFile.empty()) {
                                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Currently Redirecting To: %s", targetMod.targetFile.c_str());
                                } else {
                                    ImGui::TextDisabled("Currently using natural filename (no target assigned).");
                                }
                                ImGui::Separator();
                                ImGui::Spacing();

                                auto teamPresets = Features::VFS::GetTeamPresets();

                                // Option 1: Team Preset Dropdowns
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Option 1: Choose Team Slot");
                                
                                if (s_SelectedTeamIdx >= (int)teamPresets.size()) s_SelectedTeamIdx = 0;
                                const char* currentTeamName = teamPresets[s_SelectedTeamIdx].teamName.c_str();

                                ImGui::SetNextItemWidth(260.0f);
                                if (ImGui::BeginCombo("Team", currentTeamName)) {
                                    for (int t = 0; t < (int)teamPresets.size(); t++) {
                                        bool isSelected = (s_SelectedTeamIdx == t);
                                        if (ImGui::Selectable(teamPresets[t].teamName.c_str(), isSelected)) {
                                            s_SelectedTeamIdx = t;
                                            s_SelectedSlotIdx = 0;
                                        }
                                        if (isSelected) ImGui::SetItemDefaultFocus();
                                    }
                                    ImGui::EndCombo();
                                }

                                const auto& slots = teamPresets[s_SelectedTeamIdx].slots;
                                if (s_SelectedSlotIdx >= (int)slots.size()) s_SelectedSlotIdx = 0;
                                const char* currentSlotLabel = slots[s_SelectedSlotIdx].label.c_str();

                                ImGui::SetNextItemWidth(260.0f);
                                if (ImGui::BeginCombo("Asset Slot", currentSlotLabel)) {
                                    for (int s = 0; s < (int)slots.size(); s++) {
                                        bool isSelected = (s_SelectedSlotIdx == s);
                                        if (ImGui::Selectable(slots[s].label.c_str(), isSelected)) {
                                            s_SelectedSlotIdx = s;
                                        }
                                        if (isSelected) ImGui::SetItemDefaultFocus();
                                    }
                                    ImGui::EndCombo();
                                }

                                ImGui::Spacing();
                                if (ImGui::Button("Apply Team Slot", ImVec2(180, 26))) {
                                    vfs.SetModTarget((size_t)s_TargetingModIndex, slots[s_SelectedSlotIdx].targetFile);
                                    ImGui::CloseCurrentPopup();
                                }

                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();

                                // Option 2: Common Game Asset Slots
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Option 2: Common Global Slots");
                                if (ImGui::Button("Title Screen (titlepage.iff)")) {
                                    vfs.SetModTarget((size_t)s_TargetingModIndex, "titlepage.iff");
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGui::SameLine();
                                if (ImGui::Button("Bootup (bootup.iff)")) {
                                    vfs.SetModTarget((size_t)s_TargetingModIndex, "bootup.iff");
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGui::SameLine();
                                if (ImGui::Button("Basketball (nbaball.iff)")) {
                                    vfs.SetModTarget((size_t)s_TargetingModIndex, "nbaball.iff");
                                    ImGui::CloseCurrentPopup();
                                }

                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();

                                // Option 3: Manual Slot Entry
                                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Option 3: Manual Slot Entry (.iff)");
                                ImGui::SetNextItemWidth(200.0f);
                                ImGui::InputTextWithHint("##ManualTargetInput", "e.g. f014.iff, png1013.iff", s_CustomTargetInput, sizeof(s_CustomTargetInput));
                                ImGui::SameLine();
                                if (ImGui::Button("Apply Custom")) {
                                    if (s_CustomTargetInput[0] != '\0') {
                                        vfs.SetModTarget((size_t)s_TargetingModIndex, s_CustomTargetInput);
                                        ImGui::CloseCurrentPopup();
                                    }
                                }

                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();

                                // Bottom Actions
                                if (!targetMod.targetFile.empty()) {
                                    if (ImGui::Button("Clear Target (Use Original Name)")) {
                                        vfs.ClearModTarget((size_t)s_TargetingModIndex);
                                        ImGui::CloseCurrentPopup();
                                    }
                                    ImGui::SameLine();
                                }

                                if (ImGui::Button("Cancel", ImVec2(90, 0))) {
                                    ImGui::CloseCurrentPopup();
                                }
                            } else {
                                if (ImGui::Button("Close")) {
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                            ImGui::EndPopup();
                        }

                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tip: Unchecked mods fallback to vanilla game assets. Custom targets take effect on next asset load.");
                    }

                    ImGui::EndTabItem();
                }

                // Tab: About
                if (ImGui::BeginTabItem("About")) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "NBA 2K14 Native Hook v1.1");
                    ImGui::Text("Developed for high-performance in-game feature injection and modding.");
                    ImGui::Separator();
                    ImGui::BulletText("Direct3D 9Ex EndScene & Reset Detours");
                    ImGui::BulletText("Virtual File System (VFS) with Per-Mod Toggle & O(1) Cache");
                    ImGui::BulletText("Dear ImGui 1.90+ Integration");
                    ImGui::BulletText("MinHook Trampoline Disassembler Engine");
                    ImGui::BulletText("DirectInput 8 System Proxy Interceptor");
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
}
