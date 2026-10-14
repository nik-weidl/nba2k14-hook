#include "menu.h"
#include "../features/feature_manager.h"
#include "../features/vfs.h"
#include "../core/logger.h"
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <string>
#include <algorithm>

namespace UI {
    bool Menu::s_Initialized = false;
    HWND Menu::s_Hwnd = nullptr;
    static char s_ModSearchFilter[128] = "";

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

        // 1. Watermark / Small HUD (Always visible)
        if (settings.showFpsOverlay) {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.60f);
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
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "| %zu Mods Active", vfs.GetModCount());
                    ImGui::SameLine();
                }
                ImGui::TextDisabled("| [F8] Menu");
                ImGui::End();
            }
        }

        // 2. Main Menu Window (Toggled via F8 or Insert)
        if (!settings.showMenu) return;

        ImGui::SetNextWindowSize(ImVec2(720, 520), ImGuiCond_FirstUseEver);
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
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "ONLINE (%zu active files, %zu redirected)", vfs.GetModCount(), vfs.GetTotalRedirections());
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
                    ImGui::Checkbox("Show FPS & Status Overlay", &settings.showFpsOverlay);

                    ImGui::EndTabItem();
                }

                // Tab: Gameplay
                if (ImGui::BeginTabItem("Gameplay")) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Gameplay Overrides & Mechanics");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Checkbox("Game Speed Multiplier", &settings.speedHackEnabled);
                    if (settings.speedHackEnabled) {
                        ImGui::SliderFloat("Speed Scale", &settings.gameSpeed, 0.1f, 3.0f, "%.2fx");
                        if (ImGui::Button("Reset to Normal (1.0x)")) {
                            settings.gameSpeed = 1.0f;
                        }
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Checkbox("Unlimited Player Stamina (Zero Fatigue)", &settings.unlimitedStamina);
                    ImGui::Checkbox("Perfect Shot Timing Assist", &settings.shotMeterAlwaysGreen);
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
                    ImGui::TextWrapped("Drop loose .iff, .bin, or audio files into the 'mods/' folder. The VFS transparently redirects file-open calls at runtime so vanilla archives never need to be unpacked or modified.");
                    ImGui::Separator();
                    ImGui::Spacing();

                    bool vfsEnabled = vfs.IsEnabled();
                    if (ImGui::Checkbox("Enable /mods/ Folder Redirection", &vfsEnabled)) {
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

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Active Mods: %zu  |  Total Redirections: %zu", vfs.GetModCount(), vfs.GetTotalRedirections());
                    ImGui::TextDisabled("Location: %s", vfs.GetModsDirectory().c_str());
                    ImGui::Spacing();

                    const auto& modList = vfs.GetLoadedMods();
                    if (modList.empty()) {
                        ImGui::BeginChild("EmptyModsBox", ImVec2(0, 150), true);
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "No loose mod files found in /mods/ yet!");
                        ImGui::Spacing();
                        ImGui::BulletText("Click 'Open Mods Folder' above.");
                        ImGui::BulletText("Drop any modded files (e.g. cyberfaces, jerseys, courts) inside.");
                        ImGui::BulletText("Click 'Rescan Folder' to register them immediately.");
                        ImGui::EndChild();
                    } else {
                        ImGui::InputTextWithHint("##ModSearch", "Filter by filename...", s_ModSearchFilter, sizeof(s_ModSearchFilter));
                        ImGui::Spacing();

                        if (ImGui::BeginTable("ModsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200))) {
                            ImGui::TableSetupColumn("File Name", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                            ImGui::TableHeadersRow();

                            std::string searchLower = s_ModSearchFilter;
                            std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

                            for (const auto& mod : modList) {
                                std::string fnLower = mod.filename;
                                std::transform(fnLower.begin(), fnLower.end(), fnLower.begin(), ::tolower);

                                if (!searchLower.empty() && fnLower.find(searchLower) == std::string::npos) {
                                    continue;
                                }

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%s", mod.relativePath.c_str());

                                ImGui::TableSetColumnIndex(1);
                                if (mod.fileSize >= 1024 * 1024) {
                                    ImGui::Text("%.2f MB", (float)mod.fileSize / (1024.0f * 1024.0f));
                                } else {
                                    ImGui::Text("%.1f KB", (float)mod.fileSize / 1024.0f);
                                }

                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "Redirecting");
                            }
                            ImGui::EndTable();
                        }
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
                    ImGui::BulletText("Virtual File System (VFS) with O(1) Cache");
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
